/*
    Copyright �1994-1995, Juri Munkki
    All rights reserved.

    File: CCapKey.c
    Created: Friday, November 11, 1994, 21:17
    Modified: Tuesday, December 19, 1995, 12:18
*/

#include "CCapKey.h"
#include "CCapMaster.h"
#include "CCapFunc.h"

void	CCapKey::ICapKey(
	short		keyCode,
	CCapMaster	*theMaster)
{
	itsMaster = theMaster;
	virtualKeyCode = keyCode;
	numLabelSpots = 0;
	theKeyRegion = NewRgn();
	funcCount = 0;
	
	label[0] = 1;
	label[1] = '?';
}

void	CCapKey::ResetCap(void)
{
	SetEmptyRgn(theKeyRegion);
	label[0] = 1;
	label[1] = '?';
	numLabelSpots = 0;
}

void	CCapKey::SetKeyChar(
	char	theChar)
{
	label[0] = 1;
	label[1] = theChar;
}

void	CCapKey::SetKeyString(
	StringPtr	theString)
{
	label[0] = theString[0];
	if(label[0] > LABELMAXLEN)
	{	label[0] = LABELMAXLEN;
	}
	
	BlockMoveData(theString+1, label+1, label[0]);
}

void	CCapKey::AddKeyArea(
	short		x,
	short		y,
	RgnHandle	theArea)
{
	if(numLabelSpots < MAXLABELSPOTS)
	{	labelSpots[numLabelSpots].h = x;
		labelSpots[numLabelSpots].v = y;
		numLabelSpots++;
		
		UnionRgn(theArea, theKeyRegion, theKeyRegion);
	}
}

void	CCapKey::OffsetKey(
	short	x,
	short	y)
{
	short	i;
	
	for(i=0;i<numLabelSpots;i++)
	{	labelSpots[i].h += x;
		labelSpots[i].v += y;
	}

	OffsetRgn(theKeyRegion, x, y);
}

void	CCapKey::DrawKey()
{
	short	i,j;
	GrafPtr	port;
	Boolean	hadFunc = false;
	
	GetPort(&port);
	
	EraseRgn(theKeyRegion);
	FrameRgn(theKeyRegion);

	if(funcCount > 0)
	{	for(i=0;i<numLabelSpots;i++)
		{	for(j = 0; j < funcCount; j++)
			{	CCapFunc	*theFunc;
			
				theFunc = itsMaster->funcs[funcList[j]];
				if(theFunc)
				{	hadFunc = true;
					theFunc->DrawIcon(labelSpots[i].h, labelSpots[i].v);
				}
			}
		}
	}
	
	if(!hadFunc)
	{	short	w;
		short	savedSize;
		short	yOffset = 4;

		if(label[0] > 1)
		{	savedSize = thePort->txSize;
			TextSize(9);
			w = StringWidth((StringPtr)label) / 2 - 1;
		}
		else
		{	w = StringWidth((StringPtr)label) / 2;
			yOffset++;
		}

		for(i=0;i<numLabelSpots;i++)
		{
			MoveTo(labelSpots[i].h - w, labelSpots[i].v + yOffset);
			DrawString((StringPtr)label);
		}
		if(label[0] > 1)
		{	TextSize(savedSize);
		}

	}
}

void	CCapKey::Dispose()
{
	DisposeRgn(theKeyRegion);
	
	delete this;
}

void	CCapKey::AddFunc(
	CCapFunc	*theFunc)
{
	short	i;
	
	for(i=0;i<funcCount;i++)
	{	if(funcList[i] == theFunc->id)
		{	i = -1;
			break;
		}
	}
	
	if(i >= 0)
	{	for(i=0;i<funcCount;i++)
		{	if(itsMaster->funcs[funcList[i]]->mask & theFunc->mask)
			{	funcList[i] = funcList[--funcCount];
				i--;
			}
		}
		
		if(funcCount < MAXFUNCSPERKEY)
		{	funcList[funcCount++] = theFunc->id;
		}
	}
}

void	CCapKey::RemoveFunc(
	CCapFunc	*theFunc)
{
	short	i;
	
	for(i=0;i<funcCount;i++)
	{	if(funcList[i] == theFunc->id)
		{	funcList[i] = funcList[--funcCount];
			i--;
		}
	}
}

void	CCapKey::DragFunc(
	CCapFunc	*theFunc,
	Point		where,
	RgnHandle	dragRegion)
{
	RgnHandle	fromRegion;
	
	fromRegion = NewRgn();
	
	if(this == itsMaster->currentCap)
	{	RectRgn(fromRegion, &itsMaster->infoRect);
	}
	
	UnionRgn(fromRegion, theKeyRegion, fromRegion);
	
	if(theFunc->DoDrag(itsMaster, fromRegion, dragRegion, where))
	{
		InvalRgn(fromRegion);
		RemoveFunc(theFunc);
	}
	else
	{	itsMaster->currentCap = this;
		InvalRect(&itsMaster->infoRect);
	}

	DisposeRgn(fromRegion);
}

void	CCapKey::ReadCaps(
	long	*map)
{
	short			i;
	unsigned long	mask = 1L<<31;
	
	funcCount = 0;
	for(i=0;i<itsMaster->numFuncs;i++)
	{	if(*map & mask)
		{	funcList[funcCount++] = i;
			if(funcCount == MAXFUNCSPERKEY)
				break;
		}
		
		mask >>= 1;
		if(mask == 0)
		{	mask = 1L<<31;
			map++;
		}
	}		
}
void	CCapKey::WriteCaps(
	long	*map)
{
	short	i;
	
	for(i=0;i<FUNCLONGS;i++)	map[i] = 0;

	for(i=0;i<funcCount;i++)
	{	short			w;
		unsigned long	mask;
		
		w = funcList[i];
		mask = 1L << (31 - (w & 31));
		w >>= 5;
		
		map[w] |= mask;
	}
}