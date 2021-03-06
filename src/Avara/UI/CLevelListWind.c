/*
    Copyright �1994-1995, Juri Munkki
    All rights reserved.

    File: CLevelListWind.c
    Created: Wednesday, November 30, 1994, 8:54
    Modified: Monday, August 21, 1995, 4:26
*/

#include "CLevelListWind.h"
#include "CCompactTagBase.h"
#include "CStringDictionary.h"
#include "CAvaraApp.h"
#include "Palettes.h"
#include "CommandList.h"
#include "CNetManager.h"
#include "CLevelDescriptor.h"
#include "Script.h"
#include "PlayerConfig.h"

#define	infoSTRS	129


#define		HILITEMODE	LMSetHiliteMode(127 & LMGetHiliteMode())

enum	{	kLevelTextInd = 1,
			kPlayersTextInd,
			kTimeTextInd,
			kScoreTextInd,
			kWhenTextInd,
			kFirstTextInd,
			kFastTextInd,
			kHighTextInd,
			kPromptInd,
			kPlayerNameInd,
			kPlayerNamePromptInd,
			kUnknownPlayerName,
			kGrenadesInd,
			kMissilesInd,
			kBoostersInd,
			kHullTypeInd,
			kWeaponHelpInd,
			kKgInd
		};

//	Tab stops in Pica units.
static	short	infoTabStops[] = {	6*12, 16*12, 17*12, 27*12, 32*12, 33*12 };

#define INFOWIDTH		485
#define LOGINHEIGHT		24
#define	TITLESHEIGHT	18
#define	HEADERHEIGHT	(TITLESHEIGHT+LOGINHEIGHT+49)
#define	LEVELLINEHEIGHT	12
#define	LEVELLINEINSET	2
#define	LEVELTOPADJUST	-3
#define	LEVELINFOHEIGHT	(3*LEVELLINEHEIGHT+3*LEVELLINEINSET)

#define	loginTabStop	80
#define	loginRight		240
#define	loginRow		16

#define	tagUserName		'USER'
#define	tagWinnerNames	'WINS'
#define	tagWindowRect	'RECT'
#define	tagGrenades		'GREN'
#define	tagMissiles		'MISS'
#define	tagBoosters		'BOOS'
#define	tagHull			'HULL'
#define	tagMugShot		'MUGS'

#define	leftPopupMargin	14
#define	topPopupMargin	26
#define	popupWidth		122
#define	popupHeight		17
#define	popupVSpacing	24
#define	popupHSpacing	132
#define	kKGLineOffset	10

enum	{	kLevelsAreaBox,
			kHScrollBox,
			kVScrollBox,
			kLevelBox,
			kLevelInsideBox,
			kLevelContentBox,
			kLevelDescriptorBox,
			kHeaderBox,
			kPopupBox,
			kPopupPopBox,
			kMassInfoBox,
			kMassValueBox
		};

#define	kFirstWeaponPopMenu		135

#define	popBoxHeight	14

static	MenuHandle	weaponPopMenus[kNumWeaponPopups] = { 0,0,0,0 };
static	Handle		PopProcHandle = NULL;
static	Rect		thePopRect;
static	short		maxValidPopValue;

void	CLevelListWind::GetRect(
	Rect	*theRect,
	short	partCode,
	short	index)
{
	Boolean		result = true;
	switch(partCode)
	{	case kLevelsAreaBox:
			*theRect = itsWindow->portRect;
			theRect->top += HEADERHEIGHT+1;
			theRect->bottom -= 15;
			theRect->right -= 15;
			break;
		case kHeaderBox:
			GetRect(theRect, kLevelsAreaBox, 0);
			theRect->right = itsWindow->portRect.right;
			theRect->bottom = theRect->top;
			theRect->top = theRect->bottom - TITLESHEIGHT;
			break;
		case kLevelBox:
			theRect->left = -GetCtlValue(hScroll);
			theRect->right = theRect->left+INFOWIDTH;
			theRect->top = HEADERHEIGHT+1 - GetCtlValue(vScroll) + index * LEVELINFOHEIGHT;
			theRect->bottom = theRect->top + LEVELINFOHEIGHT;
			break;
		case kLevelInsideBox:
			GetRect(theRect, kLevelBox, index);
			theRect->top++;
			theRect->bottom--;
			break;

		case kLevelContentBox:
			{	Rect	containerRect;
			
				GetRect(theRect, kLevelBox, index);
				GetRect(&containerRect, kLevelsAreaBox, 0);
				SectRect(theRect, &containerRect, theRect);
			}
			break;
		case kLevelDescriptorBox:	
			GetRect(theRect, kLevelInsideBox, index);
			theRect->top++;
			theRect->left += infoTabStops[1] - maxInfoWidth;
			break;

		case kHScrollBox:
			*theRect = itsWindow->portRect;
			theRect->left--;
			theRect->top = theRect->bottom - 15;
			theRect->right -= 14;
			theRect->bottom = theRect->top + 16;
			break;
		case kVScrollBox:
			*theRect = itsWindow->portRect;
			theRect->left = theRect->right - 15;
			theRect->top += HEADERHEIGHT;
			theRect->right = theRect->left + 16;
			theRect->bottom -= 14;
			break;
		case kPopupBox:
			theRect->left = leftPopupMargin;
			theRect->top = topPopupMargin;
			if(index & 1)	theRect->left += popupHSpacing;
			if(index & 2)	theRect->top += popupVSpacing;
			theRect->right = theRect->left + popupWidth;
			theRect->bottom = theRect->top + popupHeight;
			break;
		case kPopupPopBox:
			GetRect(theRect, kPopupBox, index);
			theRect->left += popTitleWidths[index] + 4;
			theRect->right--;
			theRect->bottom--;
			break;
		case kMassInfoBox:
			theRect->left = leftPopupMargin + 2*popupHSpacing;
			theRect->right = maxWindowWidth - 5;
			theRect->top = topPopupMargin;
			theRect->bottom = theRect->top + popupHeight + popupVSpacing;
			theRect->top += 2;
			break;
		case kMassValueBox:
			GetRect(theRect, kMassInfoBox, 0);
			theRect->bottom = theRect->top + 16;
			theRect->left = theRect->right - 64;
			break;
	}
}

void	PopInvert(
	short	ind,
	Rect	*menuRect)
{
	Rect	invBox;
	
	if(ind)
	{	invBox = *menuRect;
		invBox.top += ind * popBoxHeight - popBoxHeight;
		invBox.bottom = invBox.top + popBoxHeight;
		InvertRect(&invBox);
	}
}

void	DrawOnePop(
		MenuHandle	theMenu,
		Rect		*inRect,
		short		i,
		short		maxValid);

static
void	DrawOnePop(
		MenuHandle	theMenu,
		Rect		*inRect,
		short		i,
		short		maxValid)
{
	short	sFont;
	short	sFace;
	short	sSize;
	unsigned char	*start, *end, *pos;
	Str255	itemStr;
	short	theValue;
						
	sFont = qd.thePort->txFont;
	sFace = qd.thePort->txFace;
	sSize = qd.thePort->txSize;
	TextFont(geneva);
	TextSize(9);
		
	GetItem(theMenu, i, itemStr);
	
	theValue = 0;

	for(i=1;i<=itemStr[0];i++)
	{	if(itemStr[i] >= '0' && itemStr[i] <= '9')
		{	theValue = (theValue * 10) + itemStr[i] - '0';
		}
		else
		{	break;
		}
	}

	if(maxValid < theValue)
	{	TextMode(grayishTextOr);
	}
	else
	{	TextMode(srcOr);
	}
		
	TextFace(bold);

	start = itemStr+1;
	end = itemStr + itemStr[0];
	pos = start;
	
	while(pos <= end && *pos++ != 32);
	
	MoveTo(inRect->left + 4,
			inRect->top);
	DrawText(start, 0, pos - start);
	
	if(pos <= end)
	{	TextFace(0);

		MoveTo( inRect->right - TextWidth(pos, 0, end - pos + 1) - 4,
				inRect->top);
		DrawText(pos, 0, end - pos + 1);
	}
	
	TextFont(sFont);
	TextFace(sFace);
	TextSize(sSize);
}

static		short		gmsg;
static		MenuHandle	gtheMenu;
static		Rect		*gmenuRect;
static		Point		ghitPt;
static		short		*gtheItem;

pascal
void	WeaponPopProc(
		short		msg,
		MenuHandle	theMenu,
		Rect		*menuRect,
		Point		hitPt,
		short		*theItem)
{
	short	newItem;
	short	i,count;

	gmsg = msg;
	gtheMenu = theMenu;
	gmenuRect = menuRect;
	ghitPt = hitPt;
	gtheItem = theItem;

	switch(msg)
	{	case mDrawMsg:
			{	Rect	inRect;
			
				inRect = *menuRect;
				count = CountMItems(theMenu);
				for(i=1;i<=count;i++)
				{	inRect.top = menuRect->top + i * popBoxHeight - popBoxHeight + 11;
					DrawOnePop(theMenu, &inRect, i, maxValidPopValue);
				}
			}
			break;
		case mSizeMsg:
			(*theMenu)->menuWidth = thePopRect.right - thePopRect.left - 2;
			(*theMenu)->menuHeight = CountMItems(theMenu) * popBoxHeight;
			break;
		case mChooseMsg:
			{	
			
				if(PtInRect(hitPt, menuRect))
				{	hitPt.v -= menuRect->top;
					newItem = 1 + hitPt.v / popBoxHeight;
				}
				else
					newItem = 0;

				if(newItem)
				{	short	theValue;
					Str255	itemStr;
				
					GetItem(theMenu, newItem, itemStr);

					theValue = 0;
				
					for(i=1;i<=itemStr[0];i++)
					{	if(itemStr[i] >= '0' && itemStr[i] <= '9')
						{	theValue = (theValue * 10) + itemStr[i] - '0';
						}
						else
						{	break;
						}
					}
			
					if(maxValidPopValue < theValue)
						newItem = 0;
				}

				if(newItem != *theItem)
				{	PopInvert(*theItem, menuRect);
					PopInvert(newItem, menuRect);
				}
				*theItem = newItem;
			}
			break;
		case mPopUpMsg:
			menuRect->top = 1+hitPt.h - ((*theItem-1) * popBoxHeight);
			menuRect->bottom = menuRect->top + CountMItems(theMenu) * popBoxHeight;
			menuRect->left = thePopRect.left+1;
			menuRect->right = thePopRect.right-1;
			if(menuRect->top < LMGetMBarHeight())
				OffsetRect(menuRect, 0, LMGetMBarHeight() - menuRect->top);
			break;
	}
}


short	PopItemToNum(MenuHandle theMenu, short ind);

static
short	PopItemToNum(MenuHandle theMenu, short ind)
{
	Str255		itemStr;
	short		iCount;
	
	iCount = CountMItems(theMenu);
	if(ind >= iCount)
		return 32767;

	GetItem(theMenu, ind+1, itemStr);
	if(itemStr[1] >= '0' && itemStr[1] <= '9')
	{	short	theValue;
		short	i;
	
		theValue = 0;
		for(i=1;i<=itemStr[0];i++)
		{	if(itemStr[i] >= '0' && itemStr[i] <= '9')
			{	theValue = (theValue * 10) + itemStr[i] - '0';
			}
			else
			{	break;
			}
		}
		
		return theValue;
	}
	else
	{	return ind;
	}
}

void	InitWeaponPopups()
{
	short	i;
	
	if(PopProcHandle == NULL)
	{	PopProcHandle = NewHandle(6);
		HLock(PopProcHandle);
		*(short *)(*PopProcHandle) = 0x4ef9;
		*(void **)(2+*PopProcHandle) = WeaponPopProc;
	
		FlushDataCache();			//	Just in case.
		FlushInstructionCache();	//	This one has to be flushed.
	
		if(!weaponPopMenus[0])
		{	for(i=0;i<kNumWeaponPopups;i++)
			{	weaponPopMenus[i] = GetMenu(kFirstWeaponPopMenu+i);
				(*weaponPopMenus[i])->menuProc = PopProcHandle;
				CalcMenuSize(weaponPopMenus[i]);
				InsertMenu(weaponPopMenus[i], hierMenu);
			}
		}
	}
}

void	CLevelListWind::InvalLevelByTag(
	OSType	tag)
{
	short	i;
	
	for(i=0;i<tagCount;i++)
	{	if(	(*(OSType **)sortedTags)[i] == tag)
		{	Rect	theBox;
		
			SetPort(itsWindow);
			GetRect(&theBox, kLevelInsideBox, i);
			InvalRect(&theBox);
		}
	}
}

void	CLevelListWind::IWindowCommander(
	CCommander			*theCommander,
	CWindowCommander	**theList)
{
	short			i;
	Rect			scrollBounds = { 0, 0, 0, 0 };
	char			zeroString[1] = {0};
	PaletteHandle	pal;

	inherited::IWindowCommander(theCommander, theList);
	
	InitWeaponPopups();

	hasFile = false;
	hasGrowBox = true;
	
	misc = new CCompactTagBase;
	misc->ITagBase();

	currentDirectory = 0;
	directory = new CCompactTagBase;
	directory->ITagBase();

	levels = new CTagBase;
	levels->ITagBase();
	
	winnerNames = new CStringDictionary;
	winnerNames->IStringDictionary();

	sortedTags = NewHandle(0);

	itsWindow = GetNewCWindow(129, 0, (WindowPtr)-1);
	pal = GetNewPalette(128);
	SetPalette(itsWindow, pal, true);

	SetPort(itsWindow);
	PmBackColor(kAvaraBackColor);
	TextFont(geneva);
	TextFace(0);
	TextSize(9);
	maxWindowWidth = itsWindow->portRect.right;

	maxInfoWidth = 0;
	for(i=0;i<3;i++)
	{	GetIndString(infoString[i], infoSTRS, kFirstTextInd+i);
		infoWidth[i] = StringWidth(infoString[i]);
		if(infoWidth[i] > maxInfoWidth)
			maxInfoWidth = infoWidth[i];
	}
	
	TextFace(bold);
	for(i=0;i<kNumWeaponPopups;i++)
	{	GetIndString(popTitles[i], infoSTRS, kGrenadesInd+i);
		popTitleWidths[i] = StringWidth(popTitles[i]);
	}
	
	popSelection[kGrenadesPop] = 3;
	popSelection[kMissilesPop] = 3;
	popSelection[kBoostersPop] = 3;
	popSelection[kHullTypePop] = 1;
	
	TextFace(0);

	AddToList();

	hScroll	= NewControl(itsWindow, &scrollBounds,(StringPtr)zeroString,-1,0,0,0,scrollBarProc,0);
	vScroll = NewControl(itsWindow, &scrollBounds,(StringPtr)zeroString,-1,0,0,0,scrollBarProc,0);	

	netButtons[0] = GetNewControl(128, itsWindow);	shownButtons[0] = true;
	netButtons[1] = GetNewControl(129, itsWindow);	shownButtons[1] = true;
	netButtons[2] = GetNewControl(130, itsWindow);	shownButtons[2] = false;
	netButtons[3] = GetNewControl(131, itsWindow);	shownButtons[3] = false;

	AdjustScrollbars();
	UpdateTagBase(false);
	
	{	unsigned long	userRef;
		OSErr			iErr;
	
		iErr = GetDefaultUser(&userRef, userName);
		if(iErr)
		{	GetIndString(userName, infoSTRS, kPlayerNamePromptInd);
		}
		
		SetRect(&loginBox, loginTabStop, loginRow - 10, loginRight, loginRow + 2);
		loginText = TEStylNew(&loginBox, &loginBox);
		TESetText(userName+1 , userName[0], loginText);
		TESetSelect(0, userName[0], loginText);
		loginEnabled = true;
		loginFrame = loginBox;
		InsetRect(&loginFrame, -2, -2);
		UserNameChanged();
	}
	
	isDirty = false;
}

Boolean	CLevelListWind::CompareTags(
	long	tag1,
	long	tag2)
{
	LevelScoreRecord	*r1,*r2;
	
	r1 = levels->GetEntryPointer(tag1);
	r2 = levels->GetEntryPointer(tag2);
	return	r1->orderNumber < r2->orderNumber;
}
void	CLevelListWind::SortTags()
{
	long	key = 0;
	long	tag;
	long	*tagTable;
	
	tagCount = 0;
	
	while(-1 != levels->GetNextTag(&key))
	{	tagCount++;
	}
	
	SetHandleSize(sortedTags, sizeof(long) * tagCount);
	HLock(sortedTags);
	levels->Lock();
	
	tagTable = (long *) *sortedTags;
	
	tagCount = 0;
	key = 0;

	tag = levels->GetNextTag(&key);

	while(tag != -1)
	{	long	i;

		for(i=tagCount-1;i>=0;i--)
		{	if(CompareTags(tagTable[i], tag))
			{	tagTable[i+1] = tagTable[i];
			}
			else	break;
			
		}
		tagTable[i+1] = tag;

		tagCount++;
		tag = levels->GetNextTag(&key);
	}
	
	levels->Unlock();
	HUnlock(sortedTags);
}
void	CLevelListWind::UpdateTagBase(
	Boolean		freshFile)
{
	OSType		appDirectory;
	
	appDirectory = ((CAvaraApp *)gApplication)->currentLevelDirectory;

	if(freshFile || currentDirectory != appDirectory)
	{	Handle	newLevels;
	
		if(!freshFile)
			CurrentLevelsToDirectory();

		levels->Dispose();
		levels = new CTagBase;
		levels->ITagBase();
		
		currentDirectory = appDirectory;
		newLevels = directory->ReadHandle(currentDirectory);
		if(newLevels)
		{	levels->ConvertFromHandle(newLevels);
			DisposeHandle(newLevels);
		}
	}

	((CAvaraApp *)itsCommander)->UpdateLevelInfoTags(levels);
	SortTags();
	AdjustScrollRanges();
}

void	CLevelListWind::ValidateBars()
{
	Rect	cBounds;

	SetPort(itsWindow);
	cBounds = (*hScroll)->contrlRect;
	ValidRect(&cBounds);
	cBounds = (*vScroll)->contrlRect;
	ValidRect(&cBounds);
}

void	CLevelListWind::ScrollList(
	Boolean	adjustBars,
	short	h,
	short	v)
{
	short		deltaH;
	short		deltaV;
	RgnHandle	freshRegion;
	
	SetPort(itsWindow);
	if(adjustBars)
	{
		deltaH = GetCtlValue(hScroll);
		deltaV = GetCtlValue(vScroll);
		SetCtlValue(hScroll, deltaH+h);
		SetCtlValue(vScroll, deltaV+v);
		deltaH = deltaH - GetCtlValue(hScroll);
		deltaV = deltaV - GetCtlValue(vScroll);
	}
	else
	{	deltaH = h;
		deltaV = v;
	}
	
	GetRect(&levelsRect, kLevelsAreaBox, 0);
	freshRegion=NewRgn();
	ScrollRect(&levelsRect,deltaH,deltaV,freshRegion);
	InvalRgn(freshRegion);
	
	if(deltaH)
	{	Rect	headerRect;
	
		GetRect(&headerRect, kHeaderBox, 0);
		ScrollRect(&headerRect,deltaH,0,freshRegion);
		InvalRgn(freshRegion);
	}

	DoUpdate();

	DisposeRgn(freshRegion);

}
void	CLevelListWind::AdjustScrollRanges()
{
	short	heightNeeded;
	short	widthNeeded;
	short	currentSetting;

	GetRect(&levelsRect, kLevelsAreaBox, 0);

	currentSetting = GetCtlValue(vScroll);
	
	heightNeeded = 15 + tagCount * LEVELINFOHEIGHT + HEADERHEIGHT - itsWindow->portRect.bottom;
	if(heightNeeded < 0) heightNeeded = 0;
	SetCtlMax(vScroll, heightNeeded);

	if(currentSetting > heightNeeded)
	{	ScrollList(true, 0, currentSetting-heightNeeded);
	}

	currentSetting = GetCtlValue(hScroll);
	widthNeeded = INFOWIDTH + 14 - itsWindow->portRect.right;

	if(widthNeeded < 0) widthNeeded = 0;
	SetCtlMax(hScroll, widthNeeded);

	if(currentSetting > widthNeeded)
	{	ScrollList(true, currentSetting-widthNeeded, 0);
	}
}

void	CLevelListWind::AdjustScrollbars()
{
	Rect	windBounds;
	Rect	controlB;
	
	GetRect(&controlB, kHScrollBox, 0);
	windBounds = itsWindow->portRect;
	
	HideControl(hScroll);
	MoveControl(hScroll,controlB.left, controlB.top);
	SizeControl(hScroll,controlB.right - controlB.left,
						controlB.bottom - controlB.top);

	GetRect(&controlB, kVScrollBox, 0);
	HideControl(vScroll);
	MoveControl(vScroll,controlB.left, controlB.top);
	SizeControl(vScroll,controlB.right - controlB.left,
						controlB.bottom - controlB.top);

	if(isActive)
	{	ShowControl(hScroll);
		ShowControl(vScroll);
	}
	ValidateBars();
}

void	CLevelListWind::Dispose()
{
	directory->Dispose();
	levels->Dispose();
	misc->Dispose();
	winnerNames->Dispose();

	DisposeHandle(sortedTags);
	TEDispose(loginText);

	inherited::Dispose();
}

CloseRequestResult	CLevelListWind::CloseRequest(
	Boolean isQuitting)
{	
	short		whatToDo;
	Boolean		result = false;
	CNetManager	*theNet;
	
	theNet = ((CAvaraApp *)gApplication)->gameNet;
	if(theNet->netOwner == this)
	{	theNet->ChangeNet(kNullNet, NULL);
	}
	
	if(theNet->netOwner == this)
	{	whatToDo = kCancelCmd;
	}
	else
	{	if(isDirty)
		{	whatToDo = ConfirmSave();
		}
		else
		{	whatToDo = kNoCmd;
		}
	}
	
	if(whatToDo == kYesCmd)
	{	if(Save())
		{	Dispose();
			result = true;
		}
	}
	else if(whatToDo == kNoCmd)
	{	Dispose();
		result = true;
	}

	return result ? kDidDispose : kDontQuit;
}


Boolean	CLevelListWind::DoGrow(
	EventRecord	*theEvent)
{
	Boolean		didChange;
	Point		newSize;
	Rect		grower;
	int			index;
	Point		oldSize;
	
	oldSize.h = itsWindow->portRect.right - itsWindow->portRect.left;
	oldSize.v = itsWindow->portRect.bottom - itsWindow->portRect.top;

	SetPort(itsWindow);
	SetRect(&grower,128,15+LEVELINFOHEIGHT*2+HEADERHEIGHT,1+maxWindowWidth,4096);
	*(long *)(& newSize) = GrowWindow(itsWindow,theEvent->where,&grower);
	SizeWindow(itsWindow,newSize.h,newSize.v,0);
	InvalRect(&itsWindow->portRect);
	ClipRect(&itsWindow->portRect);
	
	didChange = (oldSize.h != newSize.h || oldSize.v != newSize.v);

	if(didChange)
	{	AdjustScrollbars();
		AdjustScrollRanges();
	}
	
	return didChange;
}

static	CLevelListWind	*currentScroll;

static
pascal	void	ScrollAction(
	ControlHandle	theControl,
	short			partCode)
{
	Rect	areaSize;
	short	dh,dv;
	
	dh = 0;
	dv = 0;
	
	areaSize = currentScroll->levelsRect;
	
	if(partCode)
	{	if(theControl == currentScroll->hScroll)
		{	switch(partCode)
			{	case inUpButton:	dh = -8;	break;
				case inDownButton:	dh = 8;	break;
				case inPageUp:
					dh = areaSize.left-areaSize.right + 32;
					break;
				case inPageDown:
					dh = areaSize.right-areaSize.left - 32;
					break;
			}
		}
		else
		{	switch(partCode)
			{	case inUpButton:	dv = -LEVELINFOHEIGHT;	break;
				case inDownButton:	dv = LEVELINFOHEIGHT;	break;
				case inPageUp:
					dv = areaSize.top-areaSize.bottom + LEVELINFOHEIGHT;
					break;
				case inPageDown:
					dv = areaSize.bottom-areaSize.top - LEVELINFOHEIGHT;
					break;
			}
		}
		currentScroll->ScrollList(true, dh,dv);
	}
}

void	CLevelListWind::LevelBoxClick(
	EventRecord	*theEvent,
	Point		where)
{
	unsigned long		scrollTime;
	short				shown = -1;
	short				newBox;
	short				i;
	Rect				otherRect;
	CLevelDescriptor	*levelList;
	Boolean				isEnabled = false;
	OSType				theTag;
	Rect				hilitedRect = { 0,0,0,0};
	Rect				levelsAreaRect;
	
	GetRect(&levelsAreaRect, kLevelsAreaBox, 0);

	levelList = ((CAvaraApp *)gApplication)->levelList;
	
	scrollTime = theEvent->when;

	do
	{	GetNextEvent(mUpMask+mDownMask, theEvent);
		where = theEvent->where;
		GlobalToLocal(&where);
		
		if(where.v > levelsRect.bottom || where.v < levelsRect.top)
		{	if(theEvent->when - scrollTime > 10)
			{	ClipRect(&levelsAreaRect);
				HILITEMODE;
				InvertRect(&hilitedRect);
				hilitedRect.left = hilitedRect.right;

				DoUpdate();
				ClipRect(&itsWindow->portRect);
				ScrollAction(vScroll, where.v > levelsRect.bottom ? inDownButton : inUpButton);
				scrollTime = theEvent->when;			
			}
		}
		
		newBox = -1;

		for(i=0;i<tagCount;i++)
		{	Rect	levelRect;
			
			GetRect(&levelRect, kLevelContentBox, i);
			if(PtInRect(where, &levelRect))
			{	newBox = i;
				break;
			}					
		}
		
		if(newBox != shown)
		{
			ClipRect(&levelsAreaRect);
			HILITEMODE;
			InvertRect(&hilitedRect);
			DoUpdate();
			hilitedRect.left = hilitedRect.right;
			if(newBox >= 0)
			{	Rect				levelRect;
				Rect				descriptorRect;
				Str255				description;
				LevelScoreRecord	*theLevel;				
			
				theLevel = levels->GetEntryPointer(((long *)*sortedTags)[i]);
				theTag = theLevel->tag;
				isEnabled = theLevel->isEnabled;
				
				if(isEnabled)
				{	
					GetRect(&hilitedRect, kLevelContentBox, newBox);
					ClipRect(&levelsRect);

					if(theLevel->showResults)
					{	levelList->FindDescriptor(theTag, description);
						if(description[0])
						{	RgnHandle	oldClip;
						
							GetRect(&descriptorRect, kLevelDescriptorBox, newBox);
							InvalRect(&descriptorRect);
						
							EraseRect(&descriptorRect);
							descriptorRect.left += 32;
							TextFont(geneva);
							TextFace(0);
							TextSize(9);
							
							TextBox(description+1, description[0], &descriptorRect, teFlushDefault);
						}
					}
					
					HILITEMODE;
					InvertRect(&hilitedRect);
				}
			}

			shown = newBox;
		}

	} while(theEvent->what != mouseUp);
	
	ClipRect(&levelsRect);
	HILITEMODE;
	InvertRect(&hilitedRect);
	ClipRect(&itsWindow->portRect);
	if(shown != -1 && isEnabled)
	{	CNetManager	*theNet;
	
		theNet = ((CAvaraApp *)gApplication)->gameNet;
		if(theNet->netOwner == this || theNet->netStatus == kNullNet
			|| (theNet->netStatus == kClientNet && !theNet->isConnected))
		{	if(theNet->netOwner != this)
			{	theNet->ChangeNet(kNullNet, this);
			}
			
			if(theNet->netOwner == this && theNet->PermissionQuery(kAllowLoadBit, 0))
			{	theNet->SendLoadLevel(theTag);
				DoCommand(kShowRosterWind);
			}
		}
	}
}

void	CLevelListWind::PopBox(
	short			i,
	EventRecord		*theEvent)
{
	long				theSelect;
	short				theItem;
	HullConfigRecord	hull;

	GetRect(&thePopRect, kPopupPopBox, i);
	LocalToGlobal((Point *)&thePopRect);
	LocalToGlobal(1+(Point *)&thePopRect);

	hull = **(HullConfigRecord **)GetResource('HULL', 128 + popSelection[kHullTypePop]);
	switch(i)
	{	case kBoostersPop:	maxValidPopValue = hull.maxBoosters;	break;
		case kMissilesPop:	maxValidPopValue = hull.maxMissiles;	break;
		case kGrenadesPop:	maxValidPopValue = hull.maxGrenades;	break;
		case kHullTypePop:	maxValidPopValue = popSelection[kHullTypePop];	break;
	}

	theSelect = PopUpMenuSelect(weaponPopMenus[i], thePopRect.top, thePopRect.left, 1+popSelection[i]);
	theItem = theSelect;
	
	if((theSelect>>16) && popSelection[i]+1 != theItem)
	{	Rect	theRect;
	
		isDirty = true;
		popSelection[i] = theItem-1;
		
		if(i == kHullTypePop)
		{	for(i = 0; i < kNumWeaponPopups; i++)
			{	GetRect(&theRect, kPopupPopBox, i);
				theRect.left++;
				theRect.right--;
				theRect.top++;
				theRect.bottom--;
				InvalRect(&theRect);
			}
		}
		else
		{	GetRect(&theRect, kPopupPopBox, i);
			theRect.left++;
			theRect.right--;
			theRect.top++;
			theRect.bottom--;
			InvalRect(&theRect);
		}

		GetRect(&theRect, kMassValueBox, 0);
		InvalRect(&theRect);
	}
}

void	CLevelListWind::ContentClick(
	EventRecord	*theEvent,
	Point		where)
{
	short			thePart;
	ControlHandle	whichControl;
	
	thePart = FindControl(where, itsWindow, &whichControl);
	
	currentScroll = this;

	if(whichControl == hScroll || whichControl == vScroll)
	{	if(thePart)
		{	if(thePart == inThumb)
			{	short	dh,dv;
			
				dh = GetCtlValue(hScroll);
				dv = GetCtlValue(vScroll);
			
				TrackControl(whichControl, where, NULL);
				
				dh = dh - GetCtlValue(hScroll);
				dv = dv - GetCtlValue(vScroll);
				ScrollList(false, dh,dv);
			}
			else
			{	TrackControl(whichControl, where, ScrollAction);
			}	
		}
	}
	else if(thePart)
	{	long	theCommand = GetCRefCon(whichControl);
	
		thePart = TrackControl(whichControl, where, NULL);
		if(thePart)
		{	DoCommand(theCommand);
		}
	}
	else
	{	if(PtInRect(where, &loginFrame))
		{	if(!loginEnabled)
			{	TEActivate(loginText);
				loginEnabled = true;
			}
			TEClick(where, 0 != (theEvent->modifiers & shiftKey), loginText);
		}
		else
		{	//	Something else was possibly hit.
			Rect	otherBox;

			GetRect(&otherBox, kLevelsAreaBox, 0);
			if(PtInRect(where, &otherBox))
			{	LevelBoxClick(theEvent, where);
			}
			else
			{	short	i;
			
				for(i=0;i<kNumWeaponPopups;i++)
				{	GetRect(&otherBox, kPopupBox, i);
					otherBox.bottom--;
					if(PtInRect(where, &otherBox))
					{
						PopBox(i, theEvent);
						break;
					}
				}
			}
		}
	}
}

Boolean	CLevelListWind::DoZoom(
	short		partCode,
	EventRecord	*theEvent)
{
	Boolean	didZoom;
	Rect	*outZoom;
	
	outZoom = 1+(Rect *)*(((WindowPeek)itsWindow)->dataHandle);
	if(outZoom->right - outZoom->left > maxWindowWidth)
	{	outZoom->right = outZoom->left + maxWindowWidth;
	}
	
	didZoom = inherited::DoZoom(partCode, theEvent);
	ClipRect(&itsWindow->portRect);

	if(didZoom)
	{	AdjustScrollbars();
		AdjustScrollRanges();
	}
	
	return didZoom;
}

void	CLevelListWind::AdjustMenus(
	CEventHandler	*master)
{
	long	scrapOffset, picLen, textLen;

	master->EnableCommand(kSaveAsCmd);
	if(isDirty || !hasFile)
	{	master->EnableCommand(kSaveCmd);
	}

	picLen = GetScrap(NULL, 'PICT', &scrapOffset);
	textLen = GetScrap(NULL, 'TEXT', &scrapOffset);

	if(loginEnabled)
	{	master->EnableCommand(kCutCmd);
		master->EnableCommand(kCopyCmd);
	}
	
	if(picLen > 0 || (textLen > 0 && loginEnabled))
	{	master->EnableCommand(kPasteCmd);
	}

	inherited::AdjustMenus(master);
}

void	CLevelListWind::SetButtonVisibility(
	short	ind,
	Boolean	visible)
{
	if(shownButtons[ind] != visible)
	{	shownButtons[ind] = visible;
		if(visible)	ShowControl(netButtons[ind]);
		else		HideControl(netButtons[ind]);
	}
}

void	CLevelListWind::UpdateNetButtons()
{
	CNetManager	*theNet = ((CAvaraApp *)gApplication)->gameNet;
	
	if(theNet->netOwner == this && theNet->netStatus != kNullNet)
	{	SetButtonVisibility(2, theNet->netStatus != kServerNet);
		SetButtonVisibility(3, theNet->netStatus == kServerNet);
		SetButtonVisibility(0, false);
		SetButtonVisibility(1, false);
	}
	else
	{
		SetButtonVisibility(0, true);
		SetButtonVisibility(1, true);
		SetButtonVisibility(2, false);
		SetButtonVisibility(3, false);
	}
}

void	CLevelListWind::PlayerPaste()
{
	long	scrapOffset, picLen;

	picLen = GetScrap(NULL, 'PICT', &scrapOffset);
	
	if(picLen > 0)
	{	if(picLen > 16384)
		{	CCommander	*saved;
		
			saved = gApplication->BeginDialog();
			Alert(300, NULL);
			gApplication->EndDialog(saved);
		}
		else
		{	Handle	mugShot;
			CAvaraApp	*theApp = (CAvaraApp *)gApplication;
	
			mugShot = NewHandle(0);
			picLen = GetScrap(mugShot, 'PICT', &scrapOffset);
			isDirty = true;
			
			misc->WriteHandle(tagMugShot, mugShot);
			
			if(theApp->gameNet->netOwner == this)
			{	theApp->gameNet->ZapMugShot(-1);
			}

			DisposeHandle(mugShot);
		}
	}
	else
	{	TEPaste(loginText);			
		UserNameChanged();
	}
}

void	CLevelListWind::DoCommand(
	long			theCommand)
{
	CAvaraApp	*theApp;
	
	theApp = (CAvaraApp *)gApplication;
	
	switch(theCommand)
	{	case kSaveCmd:
				Save();
				break;
		case kSaveAsCmd:
				SaveAs();
				break;
		case kCutCmd:
			TECut(loginText);
			UserNameChanged();
			break;
		case kCopyCmd:
			TECopy(loginText);
			UserNameChanged();
			break;
		case kPasteCmd:
			PlayerPaste();
			break;
		case kConnectToServer:
			theApp->gameNet->ChangeNet(kClientNet, this);
			break;
		case kStartServer:
			theApp->gameNet->ChangeNet(kServerNet, this);
			break;
		case kNetChangedCmd:
			UpdateNetButtons();
			break;
		case kReportNameCmd:
			if(theApp->gameNet->netOwner == this)
			{	theApp->gameNet->NameChange(userName);
			}
			break;			
			
		case kDisconnectFromServer:
		case kStopServer:
			if(theApp->gameNet->netOwner == this)
			{	theApp->gameNet->ChangeNet(kNullNet, NULL);
			}
			break;
		case kLevelDirectoryChangedCmd:
			CurrentLevelsToDirectory();
			SetPort(itsWindow);
			InvalRect(&levelsRect);
			UpdateTagBase(false);
			break;
		case kPrepareShowLevelListWind:
			PrepareForShow();
			break;
		case kGameResultAvailableCmd:
			if(theApp->gameNet->netOwner == this)
			{	StoreGameResult();
			}
			break;
		case kConfigurePlayerCmd:
			if(theApp->gameNet->netOwner == this)
			{	GetPlayerConfiguration();
			}
			break;
		case kPastePlayerPictCmd:
			if(theApp->gameNet->netOwner == this)
			{	PlayerPaste();
			}
			break;
			
		case kGiveMugShotCmd:
			if(theApp->gameNet->netOwner == this)
			{	Handle	mugPict;
			
				mugPict = misc->ReadHandle(tagMugShot);
				theApp->gameNet->StoreMugShot(mugPict);	
			}
			break;
		default:
			inherited::DoCommand(theCommand);
	}
}

void	CLevelListWind::PrepareForShow()
{
	CAvaraApp	*itsApp = (CAvaraApp *)gApplication;
	
	if(itsApp->bottomLevelListWindow)
	{	WindowPeek	otherWindow;
	
		otherWindow = (WindowPeek) itsApp->bottomLevelListWindow->itsWindow;
		do
		{
			if(otherWindow->nextWindow == (WindowPeek) itsWindow)
			{	itsApp->bottomLevelListWindow = this;
				otherWindow = NULL;
			}
			else
			{	otherWindow = otherWindow->nextWindow;
			}
		} while(otherWindow);
	}
	else
	{	itsApp->bottomLevelListWindow = this;
	}
}

void	CLevelListWind::DrawListItem(
	short	i)
{
	short				width;
	short				v;
	Rect				theRect;
	Rect				dummy;
	short				j;
	LevelScoreRecord	*theLevel;
	CLevelDescriptor	*levelList;

	levelList = ((CAvaraApp *)gApplication)->levelList;

	GetRect(&theRect, kLevelBox, i);

	if(QSectRect(&theRect, &levelsRect, &dummy))
	{	Rect	otherRect;
	
		otherRect = theRect;
		otherRect.top = otherRect.bottom-1;
		PmBackColor(kShadowGrayColor);
		EraseRect(&otherRect);

		otherRect.top++;
		otherRect.bottom++;
		PmBackColor(0);
		EraseRect(&otherRect);

		otherRect = theRect;
		otherRect.bottom = otherRect.top+1;
		EraseRect(&otherRect);

		PmBackColor(kAvaraBackColor);
		theLevel = levels->GetEntryPointer(((long *)*sortedTags)[i]);
		
		GetRect(&theRect, kLevelInsideBox, i);

		if(theLevel)
		{	Str255				description;
			
			v = theRect.top + LEVELLINEHEIGHT + LEVELLINEINSET + LEVELTOPADJUST;

			if(theLevel->isEnabled)	TextFace(bold);
			width = StringWidth(theLevel->name);
			MoveTo(theRect.left+infoTabStops[0]-width/2, v+LEVELLINEHEIGHT);
			DrawString(theLevel->name);
			TextFace(0);

			if(theLevel->showResults)
				description[0] = 0;
			else
				levelList->FindDescriptor(theLevel->tag, description);

			if(description[0])
			{	RgnHandle	oldClip;
				Rect		descriptorRect;
			
				theRect.bottom-=2;

				GetRect(&descriptorRect, kLevelDescriptorBox, i);
				EraseRect(&descriptorRect);
				descriptorRect.left += 32;
				
				TextBox(description+1, description[0], &descriptorRect, teFlushDefault);
			}
			else
			{	if(theLevel->showResults)
				{	if(theLevel->hasWon)
					{	DrawRecordLine(theRect.left, v, &theLevel->first, 0);
						DrawRecordLine(theRect.left, v, &theLevel->fast, 2);
					}
					
					DrawRecordLine(theRect.left, v, &theLevel->high, 1);
				}
			}
		}
	}
}

void	CLevelListWind::DrawRecordLine(
	short		leftEdge,
	short		baseLine,
	GameResult	*theResult,
	short		lineIndex)
{
	Str255		theString;
	StringPtr	timeTo;
	short	width;
	long	theTime;

	baseLine += LEVELLINEHEIGHT * lineIndex;
	MoveTo(leftEdge+infoTabStops[1]-infoWidth[lineIndex], baseLine);
	DrawString(infoString[lineIndex]);
	
	theTime = theResult->time;
	if(theTime >= 0)
	{	long	minutes;
		long	seconds;
	
		minutes = theTime / (60*100);
		theTime -= minutes * 60 * 100;
		seconds = theTime / 100;
		theTime -= seconds * 100;
		
		if(minutes)
		{	NumToString(minutes, theString);
			theString[++(theString[0])] = ':';
			theString[++(theString[0])] = '0' + (seconds / 10);
			theString[++(theString[0])] = '0' + (seconds % 10);
		}
		else
		{	NumToString(seconds, theString);
		}
		
		theString[++(theString[0])] = '.';
		theString[++(theString[0])] = '0' + (theTime / 10);
		theString[++(theString[0])] = '0' + (theTime % 10);
		
		width = StringWidth(theString);
		MoveTo(leftEdge+infoTabStops[3]-width, baseLine);
		DrawString(theString);
	}
	else
	{	width = 0;
	}
	
	width = infoTabStops[3] - infoTabStops[2] - width - 6;
	winnerNames->GetIndEntry(theResult->by, theString);
	
	TruncString(width, theString, smTruncEnd);
	MoveTo(leftEdge+infoTabStops[2], baseLine);
	DrawString(theString);
	
	NumToString(theResult->score, theString);
	width = StringWidth(theString);
	MoveTo(leftEdge+infoTabStops[4]-width, baseLine);
	DrawString(theString);
	
	IUDateString(theResult->when, shortDate, theString);
	timeTo = theString + theString[0] + 1;
	IUTimeString(theResult->when, false, timeTo);
	theString[0] += timeTo[0] + 1;
	timeTo[0] = ' ';
	MoveTo(leftEdge+infoTabStops[5], baseLine);
	DrawString(theString);
}

#define	WIDE_ERASE(row)		r.top = row; r.bottom = r.top+1; EraseRect(&r)
#define	WIDE_PAINT(row)		r.top = row; r.bottom = r.top+1; PaintRect(&r)

void	CLevelListWind::DrawDividingLines()
{

	//	Draw dividing lines:
	Rect	r;

	PmBackColor(0);
	PmForeColor(1);

	r.left = itsWindow->portRect.left;
	r.right = itsWindow->portRect.right;

	WIDE_PAINT(HEADERHEIGHT-TITLESHEIGHT);
	WIDE_ERASE(HEADERHEIGHT-TITLESHEIGHT+1);

	WIDE_PAINT(HEADERHEIGHT-2);
	WIDE_ERASE(HEADERHEIGHT-1);
	WIDE_PAINT(HEADERHEIGHT);

	PmBackColor(kAvaraBackColor);
}

void	CLevelListWind::DrawHeaderStrings()
{
	Str63		tempString;
	short		width;
	short		v;
	Rect		headersRect;
	RgnHandle	sClip = SaveClip();
	
	SetOrigin(GetCtlValue(hScroll),0);
	GetRect(&headersRect, kHeaderBox, 0);
	ClipRect(&headersRect);

	v = HEADERHEIGHT - 6;

	GetIndString(tempString, infoSTRS, kLevelTextInd);
	width = StringWidth(tempString);
	MoveTo(	infoTabStops[0] - width/2, v);
	DrawString(tempString);
	
	GetIndString(tempString, infoSTRS, kPlayersTextInd);
	MoveTo(	infoTabStops[2], v);
	DrawString(tempString);
	
	GetIndString(tempString, infoSTRS, kTimeTextInd);
	width = StringWidth(tempString);
	MoveTo(	infoTabStops[3] - width, v);
	DrawString(tempString);
		
	GetIndString(tempString, infoSTRS, kScoreTextInd);
	width = StringWidth(tempString);
	MoveTo(	infoTabStops[4] - width, v);
	DrawString(tempString);
		
	GetIndString(tempString, infoSTRS, kWhenTextInd);
	MoveTo(	infoTabStops[5], v);
	DrawString(tempString);

	RestoreClip(sClip);
}

void	CLevelListWind::DrawLoginArea()
{
	Str63		tempString;
	short		width;
	
	GetIndString(tempString, infoSTRS, kPlayerNameInd);
	width = StringWidth(tempString);
	
	MoveTo(loginTabStop - width - 4, loginRow);
	DrawString(tempString);
	
	MoveTo(loginTabStop + 4, loginRow);
	DrawString(tempString);
	TEUpdate(&loginBox, loginText);
	FrameRect(&loginFrame);
}

void	CLevelListWind::DrawPops()
{
	Rect				theRect;
	short				i;
	Str255				tempStr;
	short				adjustedPop[kNumWeaponPopups];
	short				maxValues[kNumWeaponPopups];
	HullConfigRecord	hull;

	TextFont(geneva);
	TextSize(9);
	TextFace(bold);
	TextMode(srcOr);
	PmBackColor(0);

	hull = **(HullConfigRecord **)GetResource('HULL', 128 + popSelection[kHullTypePop]);
	
	maxValues[kBoostersPop] = hull.maxBoosters;
	maxValues[kMissilesPop] = hull.maxMissiles;
	maxValues[kGrenadesPop] = hull.maxGrenades;
	maxValues[kHullTypePop] = popSelection[kHullTypePop];
	adjustedPop[kHullTypePop] = popSelection[kHullTypePop];

	for(i=0;i<kHullTypePop;i++)
	{	for(adjustedPop[i] = popSelection[i];
			PopItemToNum(weaponPopMenus[i], adjustedPop[i]) > maxValues[i];
			adjustedPop[i]--);
	}

#if 0
	for(adjustedPop[kBoostersPop] = popSelection[kBoostersPop];
		PopItemToNum(weaponPopMenus[kBoostersPop], adjustedPop[kBoostersPop]) > hull.maxBoosters;
		adjustedPop[kBoostersPop]--);

	for(adjustedPop[kMissilesPop] = popSelection[kMissilesPop];
		PopItemToNum(weaponPopMenus[kMissilesPop], adjustedPop[kMissilesPop]) > hull.maxMissiles;
		adjustedPop[kMissilesPop]--);

	for(adjustedPop[kGrenadesPop] = popSelection[kGrenadesPop];
		PopItemToNum(weaponPopMenus[kGrenadesPop], adjustedPop[kGrenadesPop]) > hull.maxGrenades;
		adjustedPop[kGrenadesPop]--);
#endif

	for(i=0;i<kNumWeaponPopups;i++)
	{
		GetRect(&theRect, kPopupBox, i);
		MoveTo(theRect.left, theRect.top + 12);
		DrawString(popTitles[i]);
		GetRect(&theRect, kPopupPopBox, i);
		EraseRect(&theRect);
		FrameRect(&theRect);
		MoveTo(theRect.right, theRect.top+2);
		LineTo(theRect.right, theRect.bottom);
		LineTo(theRect.left+2, theRect.bottom);
		
		theRect.left++;
		theRect.right--;
		theRect.top += 12;
		DrawOnePop(weaponPopMenus[i], &theRect, 1+adjustedPop[i], maxValues[i]);
	}
	
	TextFace(0);
	PmBackColor(kAvaraBackColor);
	GetRect(&theRect, kMassInfoBox, 0);
	GetIndString(tempStr, infoSTRS, kWeaponHelpInd);
	TextBox(tempStr+1, tempStr[0], &theRect, teJustLeft);
	
	GetRect(&theRect, kMassValueBox, 0);
	TextFace(bold);
	GetIndString(tempStr, infoSTRS, kKgInd);
	i = theRect.right - StringWidth(tempStr);
	MoveTo(i, theRect.top+kKGLineOffset);
	DrawString(tempStr);
	MoveTo(i, theRect.top+kKGLineOffset);
	
	DrawNumber((hull.mass>>16) +
				+ 4*PopItemToNum(weaponPopMenus[kBoostersPop], adjustedPop[kBoostersPop])
				+ PopItemToNum(weaponPopMenus[kMissilesPop], adjustedPop[kMissilesPop])
				+ PopItemToNum(weaponPopMenus[kGrenadesPop], adjustedPop[kGrenadesPop])
				, kDNumAlignRight);

	TextFace(0);

}
void	CLevelListWind::DrawHeaders()
{
	DrawPops();
	DrawDividingLines();
	DrawLoginArea();
	DrawHeaderStrings();
}

void	CLevelListWind::DrawContents()
{
	Rect		aClipRect;
	short		i;

	PmBackColor(kAvaraBackColor);
	
	SetOrigin(0,0);
	ClipRect(&itsWindow->portRect);
	EraseRect(&itsWindow->portRect);

	aClipRect = itsWindow->portRect;
	aClipRect.top = HEADERHEIGHT;
	ClipRect(&aClipRect);
	if(isActive)
	{	DrawGrowIcon(itsWindow);
	}
	else
	{	MoveTo(aClipRect.right - 15, aClipRect.top);
		Line(0, 2048);
		MoveTo(aClipRect.left, aClipRect.bottom-15);
		Line(maxWindowWidth, 0);
	}

	ClipRect(&itsWindow->portRect);
	DrawHeaders();
	SetOrigin(0,0);

	TextFont(geneva);
	TextSize(9);
	TextFace(0);
	
	DrawControls(itsWindow);

	ClipRect(&levelsRect);

	levels->Lock();

	for(i=0;i<tagCount;i++)
	{	DrawListItem(i);
	}
	
	levels->Unlock();

	PmForeColor(1);
	ClipRect(&itsWindow->portRect);
}

void	CLevelListWind::DoActivateEvent()
{
	if(isActive)
	{	ShowControl(hScroll);
		ShowControl(vScroll);
		ValidateBars();
		if(loginEnabled)
		{	TEActivate(loginText);
		}
	}
	else
	{	SetCursor(&qd.arrow);
		HideControl(hScroll);
		HideControl(vScroll);
		if(loginEnabled)
		{	TEDeactivate(loginText);
		}
	}
	
	inherited::DoActivateEvent();
}

void	CLevelListWind::MiscDataToTags()
{
	Handle	theStrings;
	Rect	savedPosition;
	
	theStrings = winnerNames->WriteToHandle();
	if(theStrings)
	{	misc->WriteHandle(tagWinnerNames, theStrings);
		DisposeHandle(theStrings);
	}
	misc->WriteString(tagUserName, userName);

	misc->WriteShort(tagGrenades, popSelection[kGrenadesPop]);
	misc->WriteShort(tagMissiles, popSelection[kMissilesPop]);
	misc->WriteShort(tagBoosters, popSelection[kBoostersPop]);
	misc->WriteShort(tagHull, popSelection[kHullTypePop]);

	GetGlobalWindowRect(&savedPosition);
	misc->WriteRect(tagWindowRect, &savedPosition);
}

void	CLevelListWind::TagsToMiscData()
{
	Handle	theStrings;
	Rect	savedPosition;

	theStrings = misc->ReadHandle(tagWinnerNames);
	if(theStrings)
	{	winnerNames->ReadFromHandle(theStrings);
		DisposeHandle(theStrings);
	}

	popSelection[kGrenadesPop] = misc->ReadShort(tagGrenades, popSelection[kGrenadesPop]);
	popSelection[kMissilesPop] = misc->ReadShort(tagMissiles, popSelection[kMissilesPop]);
	popSelection[kBoostersPop] = misc->ReadShort(tagBoosters, popSelection[kBoostersPop]);
	popSelection[kHullTypePop] = misc->ReadShort(tagHull, popSelection[kHullTypePop]);

	userName[0] = 0;
	misc->ReadString(tagUserName, userName);
	TESetText(userName + 1, userName[0], loginText);
	TESetSelect(0, userName[0], loginText);
	loginEnabled = false;
	
	GetGlobalWindowRect(&savedPosition);
	misc->ReadRect(tagWindowRect, &savedPosition);
	ReviveWindowRect(&savedPosition);
	AdjustScrollbars();
}

void	CLevelListWind::CurrentLevelsToDirectory()
{
	Handle				currentLevels;
	long				key = 0;
	LevelScoreRecord	*theLevel;
	long				theTag;

	if(currentDirectory)
	{	do
		{	theTag = levels->GetNextTag(&key);
			if(theTag != -1)
			{	theLevel = levels->GetEntryPointer(theTag);
				levels->SetFlags(key-1, theLevel->hasWon | theLevel->showResults);
			}
		}	while(theTag != -1);
	
		levels->ReleaseFlagged(1, 0);
		levels->GarbageCollect();

		currentLevels = levels->ConvertToHandle();
		directory->WriteHandle(currentDirectory, currentLevels);
		DisposeHandle(currentLevels);
	}
}

Boolean	CLevelListWind::Save()
{
	Boolean	wasOk = false;

	if(!hasFile)
	{	return SaveAs();
	}
	else
	{	OSErr	iErr;
		short	refNum;

		iErr = FSpOpenDF(&itsFile, fsWrPerm, &refNum);
		if(iErr)
		{	OSErrorNotify(iErr);
			return false;
		}
		else
		{	MiscDataToTags();
			CurrentLevelsToDirectory();
			UpdateTagBase(false);
			
			misc->GarbageCollect();
			directory->GarbageCollect();
			iErr = directory->WriteToFile(refNum);
			if(!iErr)	iErr = misc->WriteToFile(refNum);
			
			if(iErr)
			{	OSErrorNotify(iErr);
			}
			else
			{	wasOk = true;
				isDirty = false;
				((CAvaraApp *)gApplication)->RememberLastSavedFile(&itsFile);
			}
			
			FSClose(refNum);
			FlushVol(NULL, itsFile.vRefNum);
			
		}
	}
	
	return wasOk;
}

Boolean	CLevelListWind::SaveAs()
{
	StandardFileReply	theReply;
	Str63				thePrompt;
	Handle				titleHandle;
	OSErr				iErr;
	Boolean				wasActive;
	
	GetIndString(thePrompt, infoSTRS, kPromptInd);

	titleHandle = (Handle)((WindowPeek)itsWindow)->titleHandle;
	HLock(titleHandle);

	wasActive = isActive;
	if(wasActive)
	{	isActive = false;
		DoActivateEvent();
		DoUpdate();
	}

	StandardPutFile(thePrompt, (StringPtr)*titleHandle, &theReply);

	if(wasActive)
	{	isActive = true;
		DoActivateEvent();
	}

	HUnlock(titleHandle);
	
	if(theReply.sfGood)
	{	itsFile = theReply.sfFile;

		if(theReply.sfReplacing)
		{	iErr = FSpDelete(&itsFile);
		}
		
		iErr = FSpCreate(&itsFile, gApplicationSignature, LEVELLISTFILETYPE, theReply.sfScript);
		if(iErr)
		{	OSErrorNotify(iErr);
			return false;
		}

		hasFile = true;
		SetWTitle(itsWindow, itsFile.name);
		DoUpdate();
		return Save();
	}
	else
		return false;
}

OSErr	CLevelListWind::OpenFile(
	FSSpec	*theFile)
{
	OSErr	iErr;
	short	refNum;

	hasFile = true;
	itsFile = *theFile;
	
	iErr = FSpOpenDF(&itsFile, fsRdPerm, &refNum);
	if(!iErr)
	{
		SetWTitle(itsWindow, itsFile.name);

		iErr = directory->ReadFromFile(refNum);

		if(!iErr)
			iErr = misc->ReadFromFile(refNum);
		
		TagsToMiscData();

		UpdateTagBase(true);
		FSClose(refNum);
	}
	
	return iErr;
}

Boolean	CLevelListWind::AvoidReopen(
	FSSpec	*theFile,
	Boolean	doSelect)
{
	if(hasFile && CompareFSSpecs(theFile, &itsFile))
	{	if(doSelect)
			Select();
		return true;
	}

	return false;
}

Boolean	CLevelListWind::DoEvent(
	CEventHandler	*master,
	EventRecord		*theEvent)
{
	Boolean	result = true;

	if(isActive)
	{	switch(theEvent->what)
		{	case nullEvent:
				{	Point	where;
				
					if(loginEnabled)
					{	TEIdle(loginText);
					}

					where = theEvent->where;
					SetPort(itsWindow);
					GlobalToLocal(&where);
					if(PtInRect(where, &loginFrame))
					{	SetCursor(&gIBeamCursor);
					}
					else
					{	SetCursor(&qd.arrow);
					}
				}
				break;
			
			case keyDown:
			case autoKey:
				{	unsigned char	theChar = theEvent->message;
				
					if(loginEnabled)
					{	if(theChar >= 28 || theChar == 8)
						{	TEKey(theEvent->message, loginText);
							UserNameChanged();
						}
						else
						{	loginEnabled = false;
							TEDeactivate(loginText);
							TESetSelect(0, (*loginText)->teLength, loginText);
						}
					}
					else
					{	if(theChar == 9)	//	Tab selects.
						{	loginEnabled = true;
							TESetSelect(0, (*loginText)->teLength, loginText);
							TEActivate(loginText);
						}
						else
						{	//	Didn't handle the event.
							result = inherited::DoEvent(master, theEvent);
						}
					}
				}
				break;
				
			default:
				result = inherited::DoEvent(master, theEvent);
				break;
		}
	}
	else
		result = inherited::DoEvent(master, theEvent);
	
	return result;
}

void	CLevelListWind::UserNameChanged()
{	
	Str32		tempName;
	short		len;
	short		i;
	CNetManager	*theNet;
	Boolean		noName;
	
	theNet = ((CAvaraApp *)gApplication)->gameNet;
	
	len = (*loginText)->teLength;
	if(len > 32)	len = 32;
	BlockMoveData(*((*loginText)->hText), tempName + 1, len);
	tempName[0] = len;
	
	noName = true;
	for(i=1;i<=tempName[0];i++)
	{	if(tempName[i] != ' ')
		{	noName = false;
			break;
		}
	}
	
	if(noName)
	{	GetIndString(tempName, infoSTRS, kUnknownPlayerName);
	}
	
	for(i=0;i<=len;i++)
	{	if(tempName[i] != userName[i])
		{	isDirty = true;
			BlockMoveData(tempName, userName, tempName[0]+1);
			if(theNet->netOwner == this)
			{	theNet->NameChange(userName);
			}
			break;
		}
	}
}

void	CLevelListWind::StoreGameResult()
{
	CNetManager			*theNet;
	TaggedGameResult	*res;
	LevelScoreRecord	newRecord;
	CLevelDescriptor	*levelList;
	long				len;
	short				foundIndex;
	Boolean				needUpdate = false;
	Boolean				needFullUpdate = false;

	levelList = ((CAvaraApp *)gApplication)->levelList;
	theNet = ((CAvaraApp *)gApplication)->gameNet;
	
	res = &theNet->gameResult;
	if(res->directoryTag == currentDirectory)
	{
		len = sizeof(LevelScoreRecord);
		foundIndex = levels->ReadEntry(res->levelTag, &len, &newRecord);
		if(foundIndex < 0)
		{	needFullUpdate = true;
			levelList->FindLevelInfo(res->levelTag, &newRecord);
		}
		
		res->r.by = -1;
		
		if(res->r.time >= 0)
		{	if(!newRecord.hasWon)
			{	needUpdate = true;
				needFullUpdate = true;
				newRecord.first = res->r;
				newRecord.hasWon = true;
				newRecord.fast = res->r;
			}
			else
			if(res->r.time < newRecord.fast.time)
			{	needUpdate = true;
				newRecord.fast = res->r;
			}
		}
		
		if(res->r.score > newRecord.high.score)
		{	needUpdate = true;
			newRecord.high = res->r;
		}
		
		if(needUpdate)
		{	short	by;

			isDirty = true;
			newRecord.showResults = true;
			by = winnerNames->FindEntry(userName, -1);
			if(newRecord.high.by == -1)		newRecord.high.by = by;
			if(newRecord.fast.by == -1)		newRecord.fast.by = by;
			if(newRecord.first.by == -1)	newRecord.first.by = by;
			levels->WriteEntry(newRecord.tag, sizeof(LevelScoreRecord), &newRecord);
			
			UpdateTagBase(false);

			if(needFullUpdate)
			{	SetPort(itsWindow);
				InvalRect(&itsWindow->portRect);
			}
			else
			{	InvalLevelByTag(newRecord.tag);
			}
			
			if(gApplication->prefsBase->ReadShort(kAutoSaveScoresTag, true))
			{	Save();
			}
		}
	}
}

void	CLevelListWind::GetPlayerConfiguration()
{
	PlayerConfigRecord	*config;
	
	config = &((CAvaraApp *)gApplication)->gameNet->config;
	config->numGrenades = PopItemToNum(weaponPopMenus[kGrenadesPop], popSelection[kGrenadesPop]);
	config->numMissiles = PopItemToNum(weaponPopMenus[kMissilesPop], popSelection[kMissilesPop]);
	config->numBoosters = PopItemToNum(weaponPopMenus[kBoostersPop], popSelection[kBoostersPop]);
	config->hullType = PopItemToNum(weaponPopMenus[kHullTypePop], popSelection[kHullTypePop]);
	config->latencyTolerance = gApplication->prefsBase->ReadShort(kLatencyToleranceTag, kDefaultLatencyTolerance);
}