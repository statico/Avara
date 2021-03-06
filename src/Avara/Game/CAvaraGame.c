/*
    Copyright �1994-1996, Juri Munkki
    All rights reserved.

    File: CAvaraGame.c
    Created: Sunday, November 20, 1994, 19:09
    Modified: Tuesday, September 17, 1996, 3:22
*/

#define	doDumps		0
#define	dumpsName	"\PAT"

#define	MAINAVARAGAME
#define	TIMING_GRAIN	4
#include "CAvaraGame.h"
#include "CAvaraApp.h"
#include "CGameWind.h"
#include "CNetManager.h"
#include "CCommManager.h"
#include "CPlayerManager.h"
#include "CDepot.h"
#include "CInfoPanel.h"
#include "InfoMessages.h"
#include "CAbstractActor.h"
#include "CAbstractPlayer.h"
#include "PAKit.h"
#include "CBSPWorld.h"
#include "CSmartPart.h"
#include "CViewParameters.h"
#include "CCapMaster.h"
#include "KeyFuncs.h"
#include "AvaraDefines.h"
#include "CSoundHub.h"
#include "CSoundMixer.h"
#include "CommandList.h"
#include "CCompactTagBase.h"
#include "Parser.h"
#include "LowMem.h"
#include "CWorldShader.h"
#include "Power.h"
#include "CScoreKeeper.h"
#include "CRosterWindow.h"
#include "Sound.h"

#define	kHighShadeCount		12

#if doDumps
#include "CPictDumper.h"
#endif

static
Fixed	menuRates[] = {	-1,	//	index from 1
						0,	//	system auto
						-1,	//	dividing line
						rate11025hz,
						rate22050hz,
						rate44khz,
						-1,
						rate11khz,
						rate22khz	};

void	CAvaraGame::InitMixer(
	Boolean		silentFlag)
{
	CSoundMixer		*aMixer;
	Fixed			theRate;
	short			maxChan;
	CAvaraApp		*theApp = (CAvaraApp *)gApplication;
	Boolean			fastMachine = theApp->fastMachine;

	theRate = menuRates[gApplication->ReadShortPref(kSoundRateOptionTag, fastMachine ? 4 : 7)];
	maxChan = gApplication->ReadShortPref(kSoundChannelsOptionTag,
						fastMachine ? 4 : 2);

	if(soundHub->itsMixer)
		soundHub->itsMixer->Dispose();

	aMixer = new CSoundMixer;
	aMixer->ISoundMixer(theRate,
						((soundOutputStyle < 0) || silentFlag) ? 0 : 32,
						maxChan, soundOutputStyle > 0, sound16BitStyle,
						soundSwitches & kInterpolateToggle);
	aMixer->SetStereoSeparation(soundOutputStyle == 2);
	aMixer->SetSoundEnvironment(FIX(400),FIX(5),frameTime);
	soundHub->AttachMixer(aMixer);
	soundHub->muteFlag = (soundOutputStyle < 0);
}

void	CAvaraGame::GameOptionCommand(
	long	theCommand)
{
	CCompactTagBase	*prefs;
	
	prefs = itsApp->prefsBase;

	switch(theCommand)
	{	
		case kMuteSoundCmd:
		case kMonoSoundCmd:
		case kStereoHeadphonesCmd:
		case kStereoSpeakersCmd:
			soundOutputStyle = theCommand - kMonoSoundCmd;
			prefs->WriteShort(kMonoStereoOptionsTag, soundOutputStyle);
			break;

		case k16BitMixCmd:
			sound16BitStyle = !sound16BitStyle;
			prefs->WriteShort(k16BitSoundOptionTag, sound16BitStyle);
			break;

		case kAmbientToggleCmd:
			soundSwitches ^= kAmbientSoundToggle;
			prefs->WriteShort(kSoundSwitchesTag, soundSwitches);
			break;
		case kTuijaToggleCmd:
			soundSwitches ^= kTuijaToggle;
			prefs->WriteShort(kSoundSwitchesTag, soundSwitches);
			break;
		case kSoundInterpolateCmd:
			soundSwitches ^= kInterpolateToggle;
			prefs->WriteShort(kSoundSwitchesTag, soundSwitches);
			break;
		case kMissileSoundLoopCmd:
			soundSwitches ^= kMissileLoopToggle;
			prefs->WriteShort(kSoundSwitchesTag, soundSwitches);
			break;
		case kFootStepSoundCmd:
			soundSwitches ^= kFootStepToggle;
			prefs->WriteShort(kSoundSwitchesTag, soundSwitches);
			break;
		case kSimpleHorizonCmd:
		case kLowDetailHorizonCmd:
		case kHighDetailHorizonCmd:
			SetPort(itsWindow);
			InvalRect(&itsWindow->portRect);
			worldShader->Apply();
			break;
	}
}

void	CAvaraGame::ResetView()
{
	itsView->viewPixelDimensions.h = gameRect.right - gameRect.left;
	itsView->viewPixelDimensions.v = gameRect.bottom - gameRect.top;
	itsView->viewPixelCenter.h = (gameRect.right - gameRect.left)/2;
	itsView->viewPixelCenter.v = (gameRect.bottom - gameRect.top)/2;
	itsView->viewWidth = itsView->viewPixelDimensions.h * 23;

//	itsView->viewDistance = itsView->viewWidth * 2 / 3;

	itsView->Recalculate();
	itsView->CalculateViewPyramidCorners();
}

void	CAvaraGame::InitLocatorTable()
{
	long			i;
	ActorLocator	**p,*q;
	
	locatorListEnd.next = NULL;
	locatorListEnd.prev = NULL;
	locatorListEnd.me = NULL;
	
	q = &locatorListEnd;
	p = locatorTable;
	for(i=0;i<LOCATORTABLESIZE;i++)
	{	*p++ = q;
	}
}

void	CAvaraGame::IAvaraGame(
	WindowPtr	theWindow,
	CAvaraApp	*theApp)
{
	short			i;
	
	infoPanel = NULL;
	frameTime = 64;	//	Milliseconds.

	itsApp = theApp;

	itsNet = new CNetManager;
	itsApp->gameNet = itsNet;
	itsNet->INetManager(this);
	
	searchCount = 0;
	locatorTable = (ActorLocator **) NewPtr(sizeof(ActorLocator *) * LOCATORTABLESIZE);
	InitLocatorTable();
	
	gameStatus = kAbortStatus;
	itsWindow = theWindow;
	actorList = NULL;
	freshPlayerList = NULL;
		
	CalcGameRect();
	InitPolyWorld(&itsPolyWorld,itsWindow, &gameRect, NULL, kShareNothing, NULL);

	itsWorld = new CBSPWorld;
	itsWorld->IBSPWorld(100);
	
	hudWorld = new CBSPWorld;
	hudWorld->IBSPWorld(16);

	soundOutputStyle = itsApp->prefsBase->ReadShort(kMonoStereoOptionsTag, 0);
	sound16BitStyle = itsApp->prefsBase->ReadShort(k16BitSoundOptionTag, true);
	soundSwitches = itsApp->prefsBase->ReadShort(kSoundSwitchesTag, -1);

	soundHub = new CSoundHub;
	soundHub->ISoundHub(32, 32);
	gHub = soundHub;

	InitMixer(true);

	scoreKeeper = new CScoreKeeper;
	scoreKeeper->IScoreKeeper(this);
	
	itsDepot = new CDepot;
	itsDepot->IDepot(this);
	
	itsView = new CViewParameters;
	itsView->IViewParameters();
	itsView->hitherBound = FIX3(600);
	itsView->yonBound = LONGYON;
	itsView->horizonBound = FIX(16000);	//	16 km
	
	mapRes = GetResource(FUNMAPTYPE, FUNMAPID);

	IGameTimer();
	SetFrameTiming(frameTime/TIMING_GRAIN);

	worldShader = new CWorldShader;
	worldShader->IWorldShader(this);
	worldShader->skyShadeCount = kHighShadeCount;
	worldShader->Apply(itsView);
	
	allowBackgroundProcessing = false;

	loadedTag = 0;
	loadedDirectory = 0;
	loadedLevel[0] = 0;
	loadedDesigner[0] = 0;
	loadedInfo[0] = 0;
	loadedTimeLimit = 600;
	timeInSeconds = 0;
	simpleExplosions = false;
}

void	CAvaraGame::Dispose()
{
	short			i;
	CAbstractActor	*nextActor;

	LevelReset(false);

	scoreKeeper->Dispose();
	itsDepot->Dispose();

	while(actorList)
	{	
		nextActor = actorList->nextActor;
		actorList->Dispose();
		actorList = nextActor;
	}
	
	while(freshPlayerList)
	{	
		nextActor = freshPlayerList->nextActor;
		freshPlayerList->Dispose();
		freshPlayerList = (CAbstractPlayer *)nextActor;
	}
		
	if(itsWorld)	itsWorld->Dispose();
	if(hudWorld)	hudWorld->Dispose();
	if(itsView)		itsView->Dispose();
	if(soundHub)	soundHub->Dispose();
	if(itsNet)		itsNet->Dispose();
	if(worldShader)	worldShader->Dispose();

	DisposePolyWorld(&itsPolyWorld);
	DisposePtr((Ptr)locatorTable);

	inherited::Dispose();
}

CAbstractActor *	CAvaraGame::FindIdent(
	long			ident)
{
	CAbstractActor *theActor;
	
	if(ident)
	{	theActor = identTable[ident & (IDENTTABLESIZE-1)];
		while(theActor && theActor->ident != ident)
		{	theActor = theActor->identLink;
		}
	}
	else
		theActor = NULL;
	
	return theActor;
}

void	CAvaraGame::GetIdent(
	CAbstractActor	*theActor)
{
	CAbstractActor	**hashLink;
	
	theActor->ident = ++curIdent;
	hashLink = identTable + (curIdent & (IDENTTABLESIZE-1));
	theActor->identLink = *hashLink;
	*hashLink = theActor;
}

void	CAvaraGame::RemoveIdent(
	long		ident)
{
	CAbstractActor	*theActor;
	CAbstractActor	**backLink;
	
	if(ident)
	{	backLink = identTable + (ident & (IDENTTABLESIZE-1));
		theActor = *backLink;
	
		while(theActor && theActor->ident != ident)
		{	backLink = &theActor->identLink;
			theActor = *backLink;
		}
		
		if(theActor && theActor->ident == ident)
		{	*backLink = theActor->identLink;
			theActor->identLink = NULL;
			theActor->ident = 0;
		}
	}
}

CAbstractPlayer	*
		CAvaraGame::GetLocalPlayer()
{
	CAbstractPlayer		*thePlayer;
	
	thePlayer = playerList;
	while(thePlayer)
	{	if(thePlayer->itsManager &&
			thePlayer->itsManager->isLocalPlayer)
		{	break;
		}
	
		thePlayer = thePlayer->nextPlayer;
	}
	
	return thePlayer;
}

void	CAvaraGame::AddActor(
	CAbstractActor	*theActor)
{
	short	i;
	
	theActor->isInGame = true;
	theActor->nextActor = actorList;
	actorList = theActor;
	
	for(i=0;i<theActor->partCount;i++)
	{	itsWorld->AddPart(theActor->partList[i]);
	}

	GetIdent(theActor);
}

void	CAvaraGame::RemoveActor(
	CAbstractActor	*theActor)
{
	CAbstractActor	*anActor, **actPP;
	
	if(theActor->isInGame)
	{	actPP = &actorList;
		anActor = actorList;
		
		while(anActor)
		{	if(anActor == theActor)
			{	short	i;

				if(theActor == nextActor)
					nextActor = theActor->nextActor;

				theActor->isInGame = false;
				theActor->UnlinkLocation();
				*actPP = anActor->nextActor;
				anActor->nextActor = NULL;
				
				for(i=0;i<anActor->partCount;i++)
				{	itsWorld->RemovePart(anActor->partList[i]);
				}
				anActor = NULL;
			}
			else
			{	actPP = &(anActor->nextActor);
				anActor = anActor->nextActor;
			}
		}
	}
	
	RemoveIdent(theActor->ident);
}

void	CAvaraGame::RegisterReceiver(
	MessageRecord	*theMsg,
	MsgType			messageNum)
{
	MessageRecord	**head;
	
	if(messageNum)
	{	head = &messageBoard[messageNum & (MESSAGEHASH - 1)];
		theMsg->next = *head;
		*head = theMsg;
	}

	theMsg->triggerCount = 0;
	theMsg->messageId = messageNum;
}

void	CAvaraGame::RemoveReceiver(
	MessageRecord	*theMsg)
{
	MessageRecord	**head;

	if(theMsg->messageId)
	{	head = &messageBoard[theMsg->messageId & (MESSAGEHASH - 1)];
		
		while(*head)
		{	if(*head == theMsg)
			{	*head = theMsg->next;
				theMsg->next = NULL;
				theMsg->messageId = 0;
				theMsg->triggerCount = 0;

				break;
			}

			head = &(*head)->next;
		}
	}
}

void	CAvaraGame::FlagMessage(
	MsgType	messageNum)
{
	MessageRecord	*msgP;

	if(messageNum)
	{	msgP = messageBoard[messageNum & (MESSAGEHASH - 1)];
		
		while(msgP)
		{	if(msgP->messageId == messageNum)
			{	msgP->triggerCount++;
				msgP->owner->isActive |= kHasMessage;
			}
	
			msgP = msgP->next;
		}
	}
}

void	CAvaraGame::FlagImmediateMessage(
	MsgType	messageNum)
{
	MessageRecord	*msgP;
	MessageRecord	*nextMsgP;

	if(messageNum)
	{	msgP = messageBoard[messageNum & (MESSAGEHASH - 1)];
		
		while(msgP)
		{	nextMsgP = msgP->next;
		
			if(msgP->messageId == messageNum)
			{	msgP->triggerCount++;
				msgP->owner->isActive |= kHasMessage;
				msgP->owner->ImmediateMessage();
			}
	
			msgP = nextMsgP;
		}
	}
}

void	CAvaraGame::MessageCleanup(
CAbstractActor	*deadActor)
{
	short			i;
	MessageRecord	*msgP, *nextP;
	MessageRecord	**head;

	for(i=0;i<MESSAGEHASH;i++)
	{	head = &messageBoard[i];
		msgP = *head;

		while(msgP)
		{	nextP = msgP->next;
			if(msgP->owner == deadActor)
			{	*head = nextP;
				msgP->next = NULL;
				msgP->messageId = 0;
				msgP->triggerCount = 0;
			}
			else
			{	head = &msgP->next;
			}
			msgP = nextP;
		}
	}
}

void	CAvaraGame::Score(
	short					team,
	short					player,
	long					points,
	Fixed					energy,
	short					hitTeam,
	short					hitPlayer)
{
	short	tempSwap;
	
	if(scoreReason == ksiObjectCollision ||
		(scoreReason == ksiKillBonus &&
		lastReason == ksiObjectCollision))
	{	tempSwap = team;
		team = hitTeam;
		hitTeam = tempSwap;
		
		tempSwap = hitPlayer;
		hitPlayer = player;
		player = tempSwap;
		
		points = -points;
	}
	
	lastReason = scoreReason;

	if(hitTeam == team)
	{	points = FMul(friendlyHitMultiplier, points);
	}
	
	if(player >= 0 && player < kMaxAvaraPlayers)
	{	scores[player] += points;
	}
	
	scoreKeeper->Score(scoreReason, team, player, points, energy, hitTeam, hitPlayer);
}

void	CAvaraGame::RunFrameActions()
{
	CAbstractActor	*theActor;
	CAbstractPlayer	*thePlayer;
	long			lastPoll;

	itsDepot->FrameAction();

	postMortemList = NULL;
	theActor = actorList;
	
	lastPoll = 0;
	while(theActor)
	{	nextActor = theActor->nextActor;
		if(theActor->isActive || --(theActor->sleepTimer) == 0)
		{	theActor->FrameAction();

			if(lastPoll != timer.stepCount)
			{	itsNet->ProcessQueue();
				lastPoll = timer.stepCount;

				if(canPreSend && timer.stepCount - nextScheduledFrame >= 0)
				{	canPreSend = false;
					itsNet->FrameAction();
				}
			}
		}
		
		theActor = nextActor;
	}

	itsNet->ProcessQueue();
	if(!latencyTolerance)
		while(frameNumber > topSentFrame)
			itsNet->FrameAction();

	thePlayer = playerList;
	while(thePlayer)
	{	if(lastPoll != timer.stepCount)
		{	itsNet->ProcessQueue();
			lastPoll = timer.stepCount;
		}

		nextPlayer = thePlayer->nextPlayer;
		thePlayer->PlayerAction();
		thePlayer = nextPlayer;
	}
}

void	CAvaraGame::ChangeDirectoryFile()
{
	gHub->FreeUnusedSamples();
}

void	CAvaraGame::LevelReset(
	Boolean clearReset)
{
	short			i;
	CAbstractActor	*anActor, *nextActor;
	Size			growBytes;
	Size			freeBytes;

	gameStatus = kAbortStatus;

	gHub->FlagOldSamples();

	loadedTag = 0;
	loadedDirectory = 0;
	loadedLevel[0] = 0;
	loadedDesigner[0] = 0;
	loadedInfo[0] = 0;
	loadedTimeLimit = 600;
	timeInSeconds = 0;

	worldShader->Reset();
	if(clearReset)
		worldShader->skyShadeCount = kHighShadeCount;
	worldShader->Apply(itsView);

	freeBytes = MaxMem(&growBytes);

	SetPort(itsWindow);
	InvalRect(&itsWindow->portRect);

	itsNet->LevelReset();

	incarnatorList = NULL;
	frameNumber = 0;
	topSentFrame = -1;

	ResetView();

	itsDepot->LevelReset();

	for(i=0;i<MESSAGEHASH;i++)
	{	messageBoard[i] = NULL;
	}

	anActor = actorList;
	while(anActor)
	{	
		nextActor = anActor->nextActor;
		anActor->LevelReset();
		anActor = nextActor;
	}
	
	while(freshPlayerList)
	{	
		nextActor = freshPlayerList->nextActor;
		freshPlayerList->Dispose();
		freshPlayerList = (CAbstractPlayer *)nextActor;
	}
	
	hudWorld->DisposeParts();
	itsWorld->DisposeParts();
	
	curIdent = 0;
	for(i=0;i<IDENTTABLESIZE;i++)
	{	identTable[i] = NULL;
	}
	
	friendlyHitMultiplier = 0;
	for(i=0;i<kMaxAvaraPlayers;i++)
	{	scores[i] = 0;
	}
	
	infoPanel->LevelReset();
	
	baseLocation = NULL;

	yonList = NULL;
	scoreReason = -1;
	lastReason = -1;
	InitLocatorTable();
}
void	CAvaraGame::EndScript()
{
	short	i;
	Fixed	intensity, angle1, angle2;
	Fixed	x,y,z;

	gameStatus = kReadyStatus;
	SetPolyWorld(&itsPolyWorld);
	worldShader->Apply(itsView);
	
	itsView->ambientLight = ReadFixedVar(iAmbient);
	
	for(i=0;i<4;i++)
	{	intensity = ReadFixedVar(iLightsTable + 3 * i);
	
		if(intensity >= 2048)
		{	angle1 = ReadFixedVar(iLightsTable+1 + 3 * i);
			angle2 = ReadFixedVar(iLightsTable+2 + 3 * i);
		
			x = FMul(FDegCos(angle1), intensity);
			y = FMul(FDegSin(-angle1), intensity);
			z = FMul(FDegCos(angle2), x);
			x = FMul(FDegSin(-angle2), x);
	
			itsView->SetLightValues(i, x, y, z, kLightGlobalCoordinates);
		}
		else
		{	itsView->SetLightValues(i, 0, 0, 0, kLightOff);
		}
	}

	friendlyHitMultiplier = ReadFixedVar(iFriendlyHitMultiplier);

	ReadStringVar(iDesignerName, loadedDesigner);
	ReadStringVar(iLevelInformation, loadedInfo);
	loadedTimeLimit = ReadLongVar(iTimeLimit);
	
	groundTraction = ReadFixedVar(iDefaultTraction);
	groundFriction = ReadFixedVar(iDefaultFriction);
	gravityRatio = ReadFixedVar(iGravity);
	groundStepSound = ReadLongVar(iGroundStepSound);
	gHub->LoadSample(groundStepSound);

	itsDepot->EndScript();

	gHub->FreeOldSamples();
	
	scoreKeeper->EndScript();
}

void	CAvaraGame::AmbientHologramControl()
{
	long			ambientHolos;
	short			i;

	ambientHolos = !gApplication->ReadShortPref(kAmbientHologramsTag, true);
	
	for(i = itsWorld->GetPartCount(); i; i--)
	{	CBSPPart	*thePart;
	
		thePart = itsWorld->GetIndPart(i-1);	//	Index from 0 to n - 1
		if(thePart && thePart->userFlags & 1)
		{	thePart->isTransparent = ambientHolos;
		}
	}
}

void	CAvaraGame::ResumeActors()
{
	CAbstractActor	*theActor;
	
	playerList = NULL;
	theActor = actorList;

	while(theActor)
	{	CAbstractActor	*next;
	
		next = theActor->nextActor;
		theActor->ResumeLevel();		
		theActor = next;
	}
	
	AmbientHologramControl();
}

void	CAvaraGame::PauseActors()
{
	CAbstractActor	*theActor;
	
	playerList = NULL;
	theActor = actorList;

	while(theActor)
	{	CAbstractActor	*next;
	
		next = theActor->nextActor;
		theActor->PauseLevel();		
		theActor = next;
	}
}

void	CAvaraGame::SendStartCommand()
{
	if(gameStatus == kReadyStatus)
	{	itsNet->SendStartCommand();
	}
	else if(gameStatus == kPauseStatus)
	{	itsNet->SendResumeCommand();
	}
}

#define	MAXSKIPSINAROW	2

extern	Boolean	newWayFlag;

static	Boolean		takeShot = false;

void	CAvaraGame::ReadGamePrefs()
{
	allowBackgroundProcessing = itsApp->ReadShortPref(kBackgroundProcessTag, 0);
	moJoOptions = itsApp->ReadShortPref(kMouseJoystickTag, 0);
	sensitivity = itsApp->ReadShortPref(kMouseSensitivityTag, 0);
	simpleExplosions = itsApp->ReadShortPref(kSimpleExplosionsTag, !itsApp->fastMachine);
}

void	CAvaraGame::ResumeGame()
{
	Boolean		doStart;
	Boolean		freshMission = (gameStatus == kReadyStatus);

	resetMouse = true;
	gApplication->SetCommandParams(STATUSSTRINGSLISTID, 
					freshMission ? kmStarted : kmRestarted, false, 0);
	gApplication->BroadcastCommand(kBusyStartCmd);

	gApplication->SetCommandParams(STATUSSTRINGSLISTID, kmGatherPlayers, true, 0);
	gApplication->BroadcastCommand(kBusyStartCmd);

	ReadGamePrefs();
	doStart = itsNet->GatherPlayers(freshMission);

	if(doStart)
	{	Byte			oldCursorScale;
		short			resRef;
		long			powerGestalt;
		short			oldEventMask;

		if(freshMission)
		{	infoPanel->MessageLine(kmStarted, centerAlign);
			itsNet->AttachPlayers((CAbstractPlayer *)freshPlayerList);
			freshPlayerList = NULL;
		}
		else
		{	infoPanel->MessageLine(kmRestarted, centerAlign);
		}

		InitMixer(false);
		resRef = CurResFile();
		UseResFile(itsApp->loadedDirectoryRef);
		if(Gestalt(gestaltPowerMgrAttr,&powerGestalt))
		{	powerGestalt = 0;
		}
		
		if(powerGestalt & (1<<gestaltPMgrCPUIdle))
		{	DisableIdle();
		}	
		
		oldEventMask = LMGetSysEvtMask();
		SetEventMask(oldEventMask | keyUpMask);
		oldCursorScale = CrsrScale;
		CrsrScale = 0;

		gApplication->SetCommandParams(STATUSSTRINGSLISTID, kmSynchPlayers, true, 0);
		gApplication->BroadcastCommand(kBusyStartCmd);

		scoreKeeper->StartResume(gameStatus == kReadyStatus);

		gApplication->BroadcastCommand(kBusyEndCmd);
		gApplication->BroadcastCommand(kBusyEndCmd);
		gApplication->BroadcastCommand(kBusyEndCmd);

		SetPort(itsWindow);
		SetPolyWorld(&itsPolyWorld);
		ClearRegions();
		CalcGameRect();
		ResizeRenderingArea(&gameRect);
		ResetView();

		GameLoop();

		CrsrScale = oldCursorScale;
		ShowCursor();
		SetEventMask(oldEventMask);	

		if(powerGestalt & (1<<gestaltPMgrCPUIdle))
		{	EnableIdle();
		}
	
		UseResFile(resRef);
		InitMixer(true);
	}
	else
	{	gApplication->BroadcastCommand(kBusyEndCmd);
		gApplication->BroadcastCommand(kBusyEndCmd);
	}
		
	itsNet->UngatherPlayers();
	SetPort(itsWindow);
	InvalRect(&itsWindow->portRect);
}
void	CAvaraGame::GameLoop()
{
	long			waitCount = 0;
	long			frameCredit;
	long			frameAdvance;
	short			consecutiveSkips = 0;
	Boolean			doRosterUpdates;
	WindowPtr		tempWindow;
	GrafPtr			saved;
	Rect			tempRect;
	short			savedMenuBarHeight;

#if doDumps
	CPictDumper		*dumper;
	
	if(takeShot)
	{	dumper = new CPictDumper;
		dumper->IPictDumper(dumpsName);
	}
#endif

	doRosterUpdates = itsApp->ReadShortPref(kUpdateRosterTag, true);

	latencyTolerance = 0;
	didWait = false;
	longWait = false;
	
	topSentFrame = frameNumber-1;

	statusRequest = kPlayingStatus;
	itsNet->ResumeGame();
	scoreKeeper->PlayerIntros();

	gameStatus = kPlayingStatus;

	ResetTiming();
	ResumeActors();

	if(frameNumber == 0)
	{	FlagMessage(iStartMsg+firstVariable);
	}
	
	playersStanding = 0;
	teamsStanding = 0;

#define		MAX_ADVANCE		(3*TIMING_GRAIN)
#define		MIN_AVDVANCE	(TIMING_GRAIN/2)
#define		MAX_DEBT		(1*TIMING_GRAIN)
#define		INIT_ADVANCE	(TIMING_GRAIN)

	frameAdvance = INIT_ADVANCE;
	frameCredit = frameAdvance << 16;
	canPreSend = false;
	HideCursor();
	
	savedMenuBarHeight = LMGetMBarHeight();
	LMSetMBarHeight(0);
	GetPort(&saved);
	SetRect(&tempRect, -1000, -1000, 3000, 3000);
	tempWindow = NewWindow(NULL, &tempRect, "\P", true, 0, NULL, false, 0);
	SetPort(tempWindow);
	SetRect(&tempRect, 0, 0, 4000, 4000);
	PaintRect(&tempRect);
	SetPort(saved);

	FlushEvents(everyEvent, 0);

	while(frameNumber + latencyTolerance > topSentFrame)
		itsNet->FrameAction();

	while(statusRequest == kPlayingStatus)
	{	long	oldPlayersStanding = playersStanding;
		short	oldTeamsStanding = teamsStanding;

		didWait = false;
		longWait = false;
		veryLongWait = false;
		playersStanding = 0;
		teamsStanding = 0;
		teamsStandingMask = 0;

		if(doRosterUpdates && (frameNumber & 63) == 0)
		{	itsApp->theRosterWind->DoUpdate();
		}

		itsApp->theGameWind->DoUpdate();
		infoPanel->DoUpdate();
		soundHub->HouseKeep();
		soundTime = soundHub->ReadTime();

		SetPort(itsWindow);
		SetPolyWorld(&itsPolyWorld);
		TrackWindow();
		
		RunFrameActions();
		
		if(playersStanding == 1 && oldPlayersStanding > 1)
		{	FlagMessage(iWin+firstVariable);
		}

		if(teamsStanding == 1 && oldTeamsStanding > 1)
		{	FlagMessage(iWinTeam+firstVariable);
		}
		
		frameNumber++;

		timeInSeconds = FMulDivNZ(frameNumber, frameTime, 1000);		

		itsNet->AutoLatencyControl(frameNumber, longWait);

		if(latencyTolerance)
			while(frameNumber + latencyTolerance > topSentFrame)
				itsNet->FrameAction();

		canPreSend = true;
		nextScheduledFrame = timer.stepCount + TIMING_GRAIN;
	
		itsDepot->RunSliverActions();
		infoPanel->StartFrame(frameNumber);
		itsNet->ViewControl();

		SetPort(itsWindow);
		SetPolyWorld(&itsPolyWorld);

		if(didWait)
		{	
			if(waitCount-- > 0)
			{	waitCount = 32;
				timer.currentStep++;
			}
		}
		newWayFlag = veryLongWait;

//		infoPanel->SetScore(timer.currentStep - timer.stepCount);

		if(didWait)
		{	if(veryLongWait)
			{	frameCredit >>= 1;
			}
			else
			{	if(longWait)
				{	frameCredit -= frameCredit >> 3;
				}
				else
				{	frameCredit -= frameCredit >> 4;
				}
			}
		}
		else
		{	frameCredit += FIX3(125);
			if(frameCredit > FIX(MAX_ADVANCE))
				frameCredit = FIX(MAX_ADVANCE);
		}

		if(		veryLongWait
			||	(consecutiveSkips == MAXSKIPSINAROW)
			|| 	(timer.currentStep + longWait >= timer.stepCount))
		{	long	targetAdvance;
		
			worldShader->ShadeWorld(itsView);
			itsWorld->Render(itsView);
			hudWorld->Render(itsView);
			PolygonizeVisRegion(itsWindow->visRgn);
			RenderPolygons();

			consecutiveSkips = 0;
			

			if(veryLongWait)
			{	frameAdvance = INIT_ADVANCE;
				frameCredit = frameAdvance << 16;
				timer.currentStep = timer.stepCount + frameAdvance;
			}
			else
			{	targetAdvance = (frameCredit + 0x8000) >> 16;
	
				if(frameAdvance > targetAdvance && frameAdvance > MIN_AVDVANCE)
					frameAdvance--;
				else
				if(frameAdvance < targetAdvance)
					frameAdvance++;
			}

			
			if(timer.currentStep >= timer.stepCount+frameAdvance)
			{	while(timer.currentStep >= GetStepCount()+frameAdvance)
				{	itsNet->ProcessQueue();
				}
			}
		}
		else
		{	consecutiveSkips++;
			SkipRendering();

			if(timer.currentStep < timer.stepCount - MAX_DEBT)
			{	timer.currentStep = timer.stepCount - MAX_DEBT;
			}
		}

		timer.currentStep += TIMING_GRAIN;
		
		if((itsNet->activePlayersDistribution & itsNet->deadOrDonePlayers)
				== itsNet->activePlayersDistribution)
		{	statusRequest = kWinStatus;	//	Just a guess, really. StopGame will change this.
		}
		
#if doDumps
		if(takeShot && (frameNumber & 1))
		{	StopTimer();
			dumper->TakeShot(itsWindow);
			ResumeTimer();
		}
#endif
	}

	if(statusRequest == kPauseStatus)
	{	PauseActors();
	}

	gameStatus = statusRequest;
	StopGame();

	itsNet->StopGame(gameStatus);	

	if(gameStatus == kAbortStatus)
	{	infoPanel->MessageLine(kmAborted, centerAlign);
	}
	else if(gameStatus == kPauseStatus)
	{	infoPanel->ParamLine(kmPaused, centerAlign, itsNet->playerTable[pausePlayer]->playerName, NULL);
	}
	
	scoreKeeper->StopPause(gameStatus == kPauseStatus);

	DisposeWindow(tempWindow);
	LMSetMBarHeight(savedMenuBarHeight);
#if doDumps
	if(takeShot)
	{	dumper->CloseDumper(true);
	}
#endif

}

void	CAvaraGame::StopGame()
{
	Rect		sizeBoxRect;
	
	soundHub->itsMixer->hushFlag = true;
	
	sizeBoxRect = itsWindow->portRect;
	sizeBoxRect.left = sizeBoxRect.right - 16;
	sizeBoxRect.top = sizeBoxRect.bottom - 16;

	SetPort(itsWindow);
	InvalRect(&sizeBoxRect);
}

void	CAvaraGame::CalcGameRect()
{
	gameRect = itsWindow->portRect;
	gameRect.top += kStatusAreaHeight;
}


void	CAvaraGame::RefreshWindow()
{
	SetPort(itsWindow);
	SetPolyWorld(&itsPolyWorld);
	TrackWindow();
	ClearRegions();
	CalcGameRect();

	ResizeRenderingArea(&gameRect);
	ResetView();

	worldShader->ShadeWorld(itsView);

	if(gameStatus == kPlayingStatus ||
		gameStatus == kPauseStatus ||
		gameStatus == kWinStatus ||
		gameStatus == kLoseStatus)
	{
		itsNet->ViewControl();
		SetPort(itsWindow);
		SetPolyWorld(&itsPolyWorld);
	
		itsWorld->Render(itsView);
		hudWorld->Render(itsView);
	}
	
	PolygonizeVisRegion(itsWindow->visRgn);
	RenderPolygons();
}

void	CAvaraGame::DrawGameWindowContents()
{
	RefreshWindow();
	//�	statusDisplay->DrawGameWindowContents();
}

CPlayerManager *CAvaraGame::GetPlayerManager(
	CAbstractPlayer	*thePlayer)
{
	CPlayerManager	*theManager = NULL;
	
	if(itsNet->playerCount < kMaxAvaraPlayers)
	{	theManager = itsNet->playerTable[itsNet->playerCount];
		theManager->SetPlayer(thePlayer);
		itsNet->playerCount++;
	}
	
	return theManager;
}

void	CAvaraGame::GotEvent()
{
	itsNet->ProcessQueue();
	if(gHub)	gHub->HouseKeep();
}


void	DebugMessage(
	StringPtr	theMessage)
{
	if(gCurrentGame->infoPanel)
	{	gCurrentGame->infoPanel->StringLine(theMessage, leftAlign);
	}
}