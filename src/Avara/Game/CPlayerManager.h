/*
    Copyright �1995-1996, Juri Munkki
    All rights reserved.

    File: CPlayerManager.h
    Created: Saturday, March 11, 1995, 5:33
    Modified: Sunday, September 15, 1996, 20:43
*/

#pragma once
#include "CDirectObject.h"
#include "KeyFuncs.h"
#include "PlayerConfig.h"

enum{
		kLNotConnected,
		kLConnected,
		kLActive,
		kLLoaded,
		kLWaiting,
		kLTrying,
		kLMismatch,
		kLNotFound,
		kLPaused,
		kLNoVehicle,
		
		kStringNorth,
		kStringSouth,
		kStringEast,
		kStringWest
	};


#define	FUNCTIONBUFFERS		32
#define	MAXFRAMEDIFFERENCE	2
#define	INPUTBUFFERSIZE		32
#define	INPUTBUFFERMASK		31

class	CAbstractPlayer;
class	CAvaraGame;
class	CRosterWindow;
class	CNetManager;

#define	kMaxMessageChars	127

class	CPlayerManager : public CDirectObject
{
public:
			CAbstractPlayer	*itsPlayer;
			CAvaraGame		*itsGame;
			CRosterWindow	*theRoster;
			CNetManager		*theNetManager;
			
			Fixed			randomKey;

			FrameFunction	frameFuncs[FUNCTIONBUFFERS];
			long			oldHeld[2];
			short			oldModifiers;
			long			askAgainTime;

			Handle			mugPict;
			long			mugSize;
			long			mugState;
			
			Point			globalLocation;
			
			char			inputBuffer[INPUTBUFFERSIZE];
			short			bufferEnd;
			short			bufferStart;
			Boolean			keyboardActive;

			short			position;
			short			spaceCount;
			Str32			playerName;
			Str63			playerRegName;
			short			isRegistered;
			unsigned char	message[kMaxMessageChars+1];
			Str255			lineBuffer;

			long			winFrame;
			short			loadingStatus;
			short			slot;
			short			playerColor;
	
			short			resetMousePeriod;
			Point			mouseCenterPosition;
			Point			oldMouse;
			Boolean			isLocalPlayer;
			
			short			levelCRC;
			OSErr			levelErr;
			OSType			levelTag;
			
			PlayerConfigRecord	theConfiguration;
			
	virtual	void			IPlayerManager(CAvaraGame *theGame, short id,
											CNetManager *aNetManager,
											CRosterWindow *aRoster);

	virtual	void			SetPlayer(CAbstractPlayer *thePlayer);

	virtual	void			DoMouseControl(Point *deltaMouse, Boolean doCenter);
#ifdef USES_KEYMAP_INSTEAD_OF_EVENTS
	virtual	void			AnalyzeKeys(EventRecord *theEvent, FunctionTable *d);
#endif
	virtual	void			EventHandler();
	virtual	void			ResumeGame();
	
	virtual	void			ProtocolHandler(struct PacketInfo *thePacket);
	virtual	void			ViewControl();
	
	virtual	FunctionTable *	GetFunctions();
	virtual	void			SendResendRequest(short askCount);

	virtual	void			Dispose();
	
	virtual	Boolean			TestHeldKey(short funcCode);

	virtual	void			FlushMessageText(Boolean forceAll);
	virtual	void			RosterMessageText(short len, char *c);

	virtual	void			RosterKeyPress(unsigned char c);
	virtual	void			GameKeyPress(char c);
	
	virtual	void			NetDisconnect();
	virtual	void			ChangeNameAndLocation(StringPtr theName, Point location);
	virtual	void			SetPosition(short pos);
	virtual	void			SetPlayerStatus(short newStatus, long theWin);

	virtual	void			ResendFrame(long theFrame, short requesterId, short commandCode);
	
	virtual	void			LoadStatusChange(short serverCRC, OSErr serverErr, OSType serverTag);
	
	virtual	CAbstractPlayer *ChooseActor(CAbstractPlayer *actorList, short myTeamColor);
	virtual	CAbstractPlayer *TakeAnyActor(CAbstractPlayer *actorList);
	virtual	Boolean			IncarnateInAnyColor();

	virtual	void			AbortRequest();
	
	virtual	void			DeadOrDone();
	
	virtual	short			GetStatusChar();
	virtual	short			GetMessageIndicator();
	
	virtual	void			StoreMugShot(Handle mugHandle);
	virtual	Handle			GetMugShot();
	
	virtual	void			GetLoadingStatusString(StringPtr theStr);

	virtual	void			SpecialColorControl();
};
