/*/
    Copyright �1996, Juri Munkki
    All rights reserved.

    File: AvaraTCP.c
    Created: Sunday, January 28, 1996, 17:24
    Modified: Tuesday, May 21, 1996, 5:29
/*/

#define MAIN_AVARA_TCP
#include "AvaraTCP.h"
#include "CExitHandler.h"
#include "CEventHandler.h"
#include "CommandList.h"

static
pascal
void	StrToAddrDone(HostInfoPtr hostInfoPtr, Boolean *doneFlag)
{
	*doneFlag = true;
}

OSErr PascalStringToAddress(
	StringPtr	name,
	ip_addr		*addr)
{
	OSErr err=noErr;
	HostInfoUPP			hiupp;
	HostInfo			theHost;
	char				cName[256];
	Boolean				lookupDone = false;
	
	
// convert host name to ip_addr

	BlockMoveData(name+1, cName, name[0]);
	cName[name[0]] = 0;
			
	hiupp = NewHostInfoProc(StrToAddrDone);
	
	err = StrToAddr(cName, &theHost, hiupp, (char *)&lookupDone);
	
	if((err == -1) || (err==cacheFault))
	{	// not done yet...
		EventRecord theEvent;
		
		while(!lookupDone)
		{	// give the system some time...
#ifndef BAD_IDEA
			gApplication->BroadcastCommand(kBusyTimeCmd);
			WaitNextEvent(0, &theEvent, 10, NULL);
#else
			if(WaitNextEvent(keyDownMask+keyUpMask+autoKeyMask, &theEvent, 10, NULL))
			{
				if(theEvent.what == keyDown)
				{	// check for command-period
					char theChar;
					
					theChar = theEvent.message;

					if(		(theEvent.modifiers & cmdKey)
						&&	(theChar == '.'))
					{
						// the user wants to cancel this operation...
						DisposeHostInfoProc(hiupp);
						return 1;
					}
				}
			}
#endif
		}
	}
	else
	if(err != noErr)
	{	// error calling StrToAddr		
		return err;
	}
	
	DisposeHostInfoProc(hiupp);
	
	// ah, the name has been converted, just take one of them out of the HostInfo record...
	*addr = theHost.addr[0];
	err = theHost.rtnCode;
	
	return err;
}

OSErr AddressToPascalString(
	ip_addr		addr,
	StringPtr	name)
{	
	OSErr err=noErr;
	HostInfoUPP			hiupp;
	HostInfo			theHost;
	char				*cName;
	Boolean				lookupDone = false;
	
	hiupp = NewHostInfoProc(StrToAddrDone);

	theHost.cname[0] = 0;

#define	USEREVERSE_MAPPING
#ifdef USEREVERSE_MAPPING
	err = AddrToName(addr, &theHost, hiupp, (char *)&lookupDone);
	
	if((err == -1) || (err==cacheFault))
	{	// not done yet...
		EventRecord theEvent;
		
		while(!lookupDone)
		{	// give the system some time...
			WaitNextEvent(0, &theEvent, 10, NULL);
		}
		
		err = theHost.rtnCode;
	}
	
	DisposeHostInfoProc(hiupp);
	
	cName = theHost.cname;
	if(err != noErr)
#endif
	{	err = AddrToStr(addr, cName);
	}

	name[0] = 0;
	while(*cName)
	{	name[++name[0]] = *cName++;
	}

	return err;
}

static	ExitRecord	tcpExit;
static	Boolean		avaraTCPIsOpen = false;

void	TCPExitFunc(
	long theData)
{
	if(avaraTCPIsOpen)
	{	gExitHandler->RemoveExit(&tcpExit);
	
		CloseResolver();
	//	KillMacTCP();
		gMacTCP = -1;
	}
}

OSErr	OpenAvaraTCP()
{
			OSErr		theErr = noErr;
	
	if(!avaraTCPIsOpen)
	{	
		theErr = OpenMacTCP(&gMacTCP, false);
		if(theErr == noErr)
		{	theErr = OpenResolver(NULL);

			tcpExit.exitFunc = TCPExitFunc;
			StartExitHandler();
			gExitHandler->AddExit(&tcpExit);
			avaraTCPIsOpen = true;
		}
	}
	
	return theErr;
}