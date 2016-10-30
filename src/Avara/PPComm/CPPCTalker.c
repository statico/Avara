/*
    Copyright �1994-1995, Juri Munkki
    All rights reserved.

    File: CPPCTalker.c
    Created: Monday, December 19, 1994, 0:22
    Modified: Monday, April 3, 1995, 23:12
*/

#include "CPPCTalker.h"
#include "GestaltEqu.h"

OSErr	CPPCTalker::IPPCTalker()
{
	OSErr	iErr;
	long	attr;
	
	GetIndString(serverKind, TALKERSTRINGS, 2);

	connectCount = 0;
	isServer = false;
	
	iErr = Gestalt(gestaltPPCToolboxAttr, &attr);
	if(iErr == noErr)
	{	if(!attr & gestaltPPCSupportsRealTime)
		{	iErr = PPCInit();	
		}
	}
	
	return iErr;
}

OSErr	CPPCTalker::Open()
{
	OSErr			iErr;
	PPCOpenPBRec	pb;
	PPCPortRec		port;
	LocationNameRec	locationName;
	
	locationName.locationKindSelector = ppcNBPTypeLocation;
	GetIndString(locationName.u.nbpType, TALKERSTRINGS, isServer ? 2 : 3);
	
	port.nameScript = 0;	//	smRoman, really, but I was too lazy to #include script.h
	GetIndString(port.name, TALKERSTRINGS, isServer ? 2 : 3);
	port.portKindSelector = ppcByString;
	GetIndString(port.u.portTypeStr, TALKERSTRINGS, 1);

	pb.ioCompletion = NULL;
	pb.serviceType = ppcServiceRealTime;
	pb.resFlag = 0;
	pb.portName = &port;
	pb.locationName = &locationName;
	pb.networkVisible = true;
	
	iErr = PPCOpen(&pb, false);
	
	theRefNum = pb.portRefNum;
	nbpRegistered = pb.nbpRegistered;
	
	return	iErr;
}

OSErr	CPPCTalker::Close()
{
	OSErr			iErr;
	PPCClosePBRec	pb;
	
	pb.ioCompletion = NULL;
	pb.portRefNum = theRefNum;
	
	iErr = PPCClose(&pb, false);
	
	return 	iErr;
}

void	CPPCTalker::InformComplete()
{
	PPCAcceptPBRec	apb;
	
	apb.ioCompletion = NULL;
	apb.sessRefNum = informE.r.sessRefNum;
	PPCAccept(&apb, true);
}

pascal
void	PPCTalkerInformFilter(
	InformEnvelope	*informP)
{
	informP->t->InformComplete();
}

OSErr	CPPCTalker::Inform()
{
	OSErr			iErr;

	informE.r.ioCompletion = (PPCCompProcPtr)PPCTalkerInformFilter;
	informE.r.portRefNum = theRefNum;
	informE.r.portName = NULL;
	informE.r.locationName = NULL;
	informE.r.userName = NULL;
	informE.r.autoAccept = false;
	informE.t = this;
	
	iErr = PPCInform(&informE.r, true);
}

pascal
Boolean	PPCTalkerBrowseFilter(
	LocationNamePtr	theLocation,
	PortInfoPtr		thePortInfo)
{
	Str32	goodName;
	Str32	goodType;
	
	GetIndString(goodName, TALKERSTRINGS, 2);
	GetIndString(goodType, TALKERSTRINGS, 1);
	return	thePortInfo->name.portKindSelector == ppcByString &&
			EqualString(goodName, thePortInfo->name.name, true, true) &&
			EqualString(goodType, thePortInfo->name.u.portTypeStr, true, true);
}


OSErr	CPPCTalker::Browse()
{
	Str255			prompt;
	LocationNameRec	locationName;
	PortInfoRec		port;
	PPCStartPBRec	pb;
	OSErr			iErr;
	
	GetIndString(prompt, TALKERSTRINGS, 4);
	
	iErr = PPCBrowser(prompt, NULL, false, &locationName, &port, PPCTalkerBrowseFilter, NULL);

	if(iErr == noErr)
	{	pb.ioCompletion = NULL;
		pb.portRefNum = theRefNum;
		pb.serviceType = ppcServiceRealTime;
		pb.resFlag = 0;
		pb.portName = &port.name;
		pb.locationName = &locationName;
		pb.userRefNum = 0;
		
		iErr = PPCStart(&pb, false);
		informE.r.sessRefNum = pb.sessRefNum;
		
	}

	return 	iErr;
}

OSErr	CPPCTalker::End()
{
	PPCEndPBRec		pb;
	OSErr			iErr;
	
	pb.ioCompletion = NULL;
	pb.sessRefNum = informE.r.sessRefNum;
	iErr = PPCEnd(&pb, false);
	
	return iErr;
}

void	CPPCTalker::AsyncRead()
{
	readE.r.ioCompletion = NULL;
	readE.r.sessRefNum = informE.r.sessRefNum;
	readE.r.bufferLength = PPCTALKERBUFFERSIZE;
	readE.r.bufferPtr = dataBuffer;
	
	PPCRead(&readE.r, true);
}

OSErr	CPPCTalker::Write(
	long	len,
	Ptr		theData)
{
	PPCWritePBRec	pb;
	
	pb.ioCompletion = NULL;
	pb.sessRefNum = informE.r.sessRefNum;
	pb.bufferLength = len;
	pb.bufferPtr = theData;
	pb.more = false;
	
	return PPCWrite(&pb, false);
}

