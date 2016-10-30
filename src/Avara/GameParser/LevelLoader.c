/*/
    Copyright �1992-1995, Juri Munkki
    All rights reserved.

    File: LevelLoader.c
    Created: Saturday, December 5, 1992, 13:29
    Modified: Wednesday, June 28, 1995, 2:17
/*/

#include "CWallActor.h"
#include "CAvaraGame.h"
#include "LevelLoader.h"
#include "Parser.h"
#include "FastMat.h"
#include "QDOffscreen.h"

#define	textBufferSize	4096

#define	POINTTOUNIT(pt)	(pt * 20480 / 9)

typedef	struct
{	long	v;
	long	h;
}	LongPoint;

static	CGrafPtr	levelPort;
static	Ptr			textBuffer;
static	long		textInBuffer;
static	LongPoint	lastArcPoint;
static	short		lastArcAngle;

static	LongPoint	lastOvalPoint;
static	long		lastOvalRadius;

		Rect		gLastBoxRect;
		short		gLastBoxRounding;

static	Rect		onePixelRect = {0,0,1,1};
static	Rect		otherPixelRect = {0,1,1,2};
static	Rect		bothPixelRect = {0,0,1,2};

static	LongPoint	lastDomeCenter;
static	short		lastDomeAngle;
static	short		lastDomeSpan;
static	long		lastDomeRadius;

static void		TextBreak()
{
	if(textInBuffer)
	{	textBuffer[textInBuffer] = 0;	//	null-terminate.
		RunThis((StringPtr)textBuffer);
		textInBuffer = 0;
	}
}


Fixed	GetDome(
	Fixed	*theLoc,
	Fixed	*startAngle,
	Fixed	*spanAngle)
{
	theLoc[0] = POINTTOUNIT(lastDomeCenter.h);
	theLoc[2] = POINTTOUNIT(lastDomeCenter.v);
	theLoc[3] = 0;
	*startAngle = FDegToOne(((long)lastDomeAngle)<<16);
	*spanAngle = FDegToOne(((long)lastDomeSpan)<<16);

	return POINTTOUNIT(lastDomeRadius);
}

long	GetPixelColor()
{
	RGBColor	color;
	
	GetCPixel( 0, 0, &color);
	
	return	((((long)color.red) << 8) & 0xFF0000) |
			(color.green & 0xFF00) |
			(color.blue >> 8);
}

long	GetOtherPixelColor()
{
	RGBColor	color;
	
	GetCPixel( 1, 0, &color);
	
	return	((((long)color.red) << 8) & 0xFF0000) |
			(color.green & 0xFF00) |
			(color.blue >> 8);

}

long	GetDecimalColor()
{

	RGBColor	color;

	GetCPixel( 0, 0, &color);
	
	return ((color.red>>8) * 1000L + (color.green>>8)) * 1000L + (color.blue>>8);
}

void	GetLastArcLocation(
	Fixed	*theLoc)
{
	theLoc[0] = POINTTOUNIT(lastArcPoint.h);
	theLoc[1] = 0;
	theLoc[2] = POINTTOUNIT(lastArcPoint.v);
	theLoc[3] = 0;
}

Fixed	GetLastOval(
	Fixed	*theLoc)
{
	theLoc[0] = POINTTOUNIT(lastOvalPoint.h);
	theLoc[1] = 0;
	theLoc[2] = POINTTOUNIT(lastOvalPoint.v);
	theLoc[3] = 0;
	
	return POINTTOUNIT(lastOvalRadius);
}

Fixed	GetLastArcDirection()
{
	return FDegToOne(((long)lastArcAngle)<<16);
}

static	Ptr		sPicDataPtr;
static	long	sPicCount;
static	Point	sRealRoundingOval;

static
pascal
void	PeekGetPic(
	Ptr		dataPtr,
	short	byteCount)
{	
	BlockMoveData(sPicDataPtr + sPicCount, dataPtr, byteCount);
	sPicCount += byteCount;

	if(byteCount == 2 && (*(short *)dataPtr) == 0x000B)
	{	sRealRoundingOval = *(Point *)(sPicDataPtr+sPicCount);
	}
}

static
pascal void		PeepStdRRect(
	GrafVerb verb,
	Rect	 *r,
	short	 ovalWidth,
	short	 ovalHeight)
{
	CWallActor	*theWall;
	
	TextBreak();
	
	if(verb == frame)
	{	short	thickness;
	
		ClipRect(&bothPixelRect);
		StdRect(paint, &otherPixelRect);

		r->left += (levelPort->pnSize.h)>>1;
		r->top += (levelPort->pnSize.v)>>1;
		r->right -= (levelPort->pnSize.h+1)>>1;;
		r->bottom -= (levelPort->pnSize.v+1)>>1;

		thickness = levelPort->pnSize.h;
		if(thickness < levelPort->pnSize.v)
			thickness = levelPort->pnSize.v;

#if 1
		ovalHeight = sRealRoundingOval.v;
		ovalWidth = sRealRoundingOval.h;
#endif
		if(ovalHeight > ovalWidth) ovalHeight = ovalWidth;
		
		if(thickness == 1)
		{	theWall = new CWallActor;
			theWall->IAbstractActor();
			theWall->MakeWallFromRect(r, ovalHeight, 0, true);
		}
		else
		{	gLastBoxRect = *r;
			gLastBoxRounding = ovalHeight;
		}
	}
	else
	{	ClipRect(&bothPixelRect);
		StdRect(verb, &onePixelRect);
	}
}

static
pascal
void	PeepStdArc(
	GrafVerb	verb,
	Rect		*r,
	short		startAngle,
	short 		arcAngle)
{
	long	r1,r2;

	TextBreak();

	ClipRect(&bothPixelRect);
	StdRect(verb, (verb == frame) ? &otherPixelRect : &onePixelRect);

	{
#if 0
		if(verb == frame)
		{	r->right--;
			r->bottom--;
		}
#else
		r->left += (levelPort->pnSize.h)>>1;
		r->top += (levelPort->pnSize.v)>>1;
		r->right -= (levelPort->pnSize.h+1)>>1;;
		r->bottom -= (levelPort->pnSize.v+1)>>1;
#endif
		r1 = r->right - r->left;
		r2 = r->bottom - r->top;
		if(r2 > r1) r1 = r2;

		lastArcPoint.h = r->left+r->right;
		lastArcPoint.v = r->top+r->bottom;
		lastArcAngle = (720 - (startAngle + arcAngle / 2)) % 360;

		lastDomeCenter.h = r->left+r->right;
		lastDomeCenter.v = r->top+r->bottom;
		lastDomeAngle = 360 - startAngle;
		lastDomeSpan = arcAngle;
		lastDomeRadius = r1;
	}
}

static
pascal
void	PeepStdOval(
	GrafVerb verb,
	Rect *r)
{
	long	r1,r2;

	TextBreak();

	ClipRect(&bothPixelRect);
	StdRect(verb, (verb == frame) ? &otherPixelRect : &onePixelRect);

	if(verb == frame)
	{	r->right--;
		r->bottom--;
		r1 = r->right - r->left;
		r2 = r->bottom - r->top;
	
		if(r2 > r1) r1 = r2;
		
		lastOvalPoint.h = r->left + r->right;
		lastOvalPoint.v = r->top + r->bottom;
		lastOvalRadius = r1 + r1;

		lastDomeCenter.h = r->left+r->right;
		lastDomeCenter.v = r->top+r->bottom;

		lastDomeAngle = 0;
		lastDomeSpan = 360;
		lastDomeRadius = r1;
	}
}

static
pascal	void	PeepStdPoly(
	GrafVerb	grafVerb,
	PolyHandle	thePoly)
{
#ifdef DONOTHING
	short		state;
	RGBColor	currentColor;
	long		longColor;
	
	TextBreak();

	state = HGetState((Handle)thePoly);
	HLock((Handle)thePoly);
	
	switch(grafVerb)
	{
		case frame:
		case invert:
		case fill:
			GetForeColor(&currentColor);
			break;
		case paint:
		case erase:
			GetBackColor(&currentColor);
			break;
	}
	
	HSetState((Handle)thePoly, state);
#endif
}

static
pascal	void	PeepStdText(
	short	byteCount,
	Ptr		theText,
	Point	numer,
	Point	denom)
{	
	short			i;
	char			*p;
	static Point	oldPoint;
	Point			newPoint;

	if(byteCount + textInBuffer + 2 < textBufferSize)
	{
		p = textBuffer+textInBuffer;
		GetPen(&newPoint);
			
		if(textInBuffer && p[-1] != 13)
		{	while(newPoint.v > oldPoint.v)
			{	FontInfo	fInfo;
			
				GetFontInfo(&fInfo);
			
				oldPoint.v += fInfo.ascent + fInfo.descent + fInfo.leading;
				*p++ = 13;
				textInBuffer++;
			}
		}
		oldPoint = newPoint;
		
		textInBuffer += byteCount;
		
		i = byteCount;
		while(i--)
		{	*p++ = *theText++;
		}
	}
}

static
pascal void PeepStdComment(
	short	kind,
	short	dataSize,
	Handle	dataHandle)
{
	switch(kind)
	{	case TextBegin:
//		case StringBegin:
			textInBuffer = 0;
			break;
			
		case TextEnd:
//		case StringEnd:
			textBuffer[textInBuffer] = 0;	//	null-terminate.
			RunThis((StringPtr)textBuffer);
			textInBuffer = 0;
			break;
	}
}

static
pascal
void	PeepStdRect(
	GrafVerb verb,
	Rect *r)
{
	CWallActor	*theWall;
	
	TextBreak();

	if(verb == frame)
	{	short	thickness;
	
		ClipRect(&bothPixelRect);
		StdRect(paint, &otherPixelRect);

		r->left += (levelPort->pnSize.h)>>1;
		r->top += (levelPort->pnSize.v)>>1;
		r->right -= (levelPort->pnSize.h+1)>>1;;
		r->bottom -= (levelPort->pnSize.v+1)>>1;

		thickness = levelPort->pnSize.h;
		if(thickness < levelPort->pnSize.v)
			thickness = levelPort->pnSize.v;

		if(thickness == 1)
		{	theWall = new CWallActor;
			theWall->IAbstractActor();
			theWall->MakeWallFromRect(r, 0, 0, true);
		}
		else
		{	gLastBoxRect = *r;
			gLastBoxRounding = 0;
		}
	}
	else
	{	ClipRect(&bothPixelRect);
		StdRect(verb, &onePixelRect);
	}
}


void	ConvertToLevelMap(
	PicHandle	thePicture)
{
	GWorldPtr	utilGWorld;
	CQDProcs	myQDProcs;
	GDHandle	savedGD;
	CGrafPtr	savedGrafPtr;
	Rect		twoPixelRect = { 0,0,1,2 };
	int			state;
	
	InitParser();
	textBuffer = NewPtr(textBufferSize);
	
	GetGWorld(&savedGrafPtr, &savedGD);
	NewGWorld(&utilGWorld, 32, &twoPixelRect, NULL, NULL, keepLocal);
	LockPixels(GetGWorldPixMap(utilGWorld));
	SetGWorld(utilGWorld, NULL);

	GetPort((GrafPtr *)&levelPort);
	SetStdCProcs(&myQDProcs);
	ClipRect(&twoPixelRect);

	myQDProcs.arcProc = (void *)PeepStdArc;
	myQDProcs.rRectProc = (void *)PeepStdRRect;
	myQDProcs.polyProc = (void *)PeepStdPoly;
	myQDProcs.textProc = (void *)PeepStdText;
	myQDProcs.rectProc = (void *)PeepStdRect;
	myQDProcs.ovalProc = (void *)PeepStdOval;
	myQDProcs.commentProc = (void *)PeepStdComment;
	myQDProcs.getPicProc = (void *)PeekGetPic;
	levelPort->grafProcs = (void *)&myQDProcs;
	
	state = HGetState((Handle) thePicture);
	HLock((Handle) thePicture);
	
	sPicDataPtr = (Ptr) (1+&((*thePicture)->picFrame));
	sPicCount = 0;
	
	DrawPicture(thePicture,&((*thePicture)->picFrame));
	TextBreak();
	
	HSetState((Handle) thePicture, state);

	levelPort->grafProcs = 0;

	SetGWorld(savedGrafPtr, savedGD);
	DisposeGWorld(utilGWorld);
	
	DisposePtr(textBuffer);
	
	FreshCalc();
	gCurrentGame->EndScript();
}

void	ConvertResToLevelMap(
	short	resId)
{
	PicHandle	thePicture;
	
	thePicture = GetPicture(resId);
	ConvertToLevelMap(thePicture);
	ReleaseResource((Handle)thePicture);
}

/*
**	The following function is for DEBUGGING PURPOSES ONLY.
**	Reading from the default directory is an extremely
**	unhealthy practice and should be avoided. On the other
**	hand, while you are developing the program, you can
**	keep the graphics in the same folder as the project 
**	a PICT file is easier to edit than a PICT resource in a
**	resource file, so this routine is convenient when you
**	still edit the pictures relatively often. When you ship
**	the application, make a PICT resource of the file and
**	put it into your application. You can then use the
**	ConvertResToLevelMap routine to do the conversion.
*/
void	ConvertFileToLevelMap(
	StringPtr	fileName)
{
	short		ref;
	short		err;
	long		len;
	PicHandle	thePicture;
	
	err = FSOpen(fileName,0,&ref);
	if(err) return;
	
	GetEOF(ref,&len);
	
	len-=512;
	if(len > 0)
	{	thePicture = (PicHandle) NewHandle(len);
		if(thePicture)
		{	HLock((Handle)thePicture);
			SetFPos (ref, fsFromStart, 512);	//	Skip PICT header

			err = FSRead(ref, &len, *thePicture);
			
			if(err == noErr)
			{	ConvertToLevelMap(thePicture);
			}
			HUnlock((Handle)thePicture);
		}
		DisposHandle((Handle)thePicture);
	}
	FSClose(ref);
}
