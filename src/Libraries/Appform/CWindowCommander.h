/*/
    Copyright �1994-1996, Juri Munkki
    All rights reserved.

    File: CWindowCommander.h
    Created: Wednesday, November 16, 1994, 1:12
    Modified: Wednesday, April 24, 1996, 1:06
/*/

#pragma once
#include "CCommander.h"

typedef 
enum	{	kDontQuit,
			kDidDispose,
			kDidHide,
			kDidHideDontDispose
		} CloseRequestResult;

enum	{	kDNumAlignRight = 1,
			kDNumAlignCenter = 2,
			kDNumPrependSpace = 4,
			kDNumAppendComma = 8,
			kDNumAppendSpace = 16,
			kDNumAppendPeriod = 32
		};

class	CWindowCommander : public CCommander
{
public:

	class	CWindowCommander	*nextWindow;
			Boolean				wantsOnWindowsMenu;
			CWindowCommander	**windowList;
			WindowPtr			itsWindow;
			Boolean				isActive;
			Boolean				canClose;
			Boolean				hasGrowBox;
	
	virtual	void				IWindowCommander(CCommander *theCommander,
												CWindowCommander **theList);
	virtual	void				AddToList();
	virtual	void				Close();
	virtual	CloseRequestResult	CloseRequest(Boolean isQuitting);

	virtual	void				Dispose();
	virtual	void				Select();

	virtual	void				RawClick(WindowPtr theWind, short partCode, EventRecord *theEvent);
	virtual	void				ContentClick(EventRecord *theEvent, Point where);
	virtual	Boolean				DoGrow(EventRecord *theEvent);
	virtual	Boolean				DoZoom(short partCode, EventRecord *theEvent);
	virtual	void				ToggleZoomState();
	
	virtual	CWindowCommander *	FindActiveWindow();
	virtual	void				DoActivateEvent();
	virtual	void				DoUpdate();
	virtual	void				DrawContents();
	
	virtual	short				ConfirmSave();
	
	virtual	Boolean				CompareFSSpecs(FSSpec *a, FSSpec *b);
	virtual	Boolean				AvoidReopen(FSSpec *theFile, Boolean doSelect);
	
	virtual	void				GetGlobalWindowRect(Rect *theRect);
	virtual	Boolean				TryWindowRect(Rect *tryRect);
	virtual	void				ReviveWindowRect(Rect *theRect);
	virtual	void				RestoreFromPreferences(OSType rectTag, OSType visTag, Boolean defaultShow, Boolean forceNewSize);
	virtual	void				SaveIntoPreferences(OSType rectTag, OSType visTag);

	virtual	void				DrawNumber(long theNum, short options);
	
	virtual	RgnHandle			SaveClip();
	virtual	void				RestoreClip();

	//	CCommander methods:
	virtual	void				AdjustMenus(CEventHandler *master);
	virtual	void				DoCommand(long theCommand);
	virtual	Boolean				DoEvent(CEventHandler *master, EventRecord *theEvent);
};