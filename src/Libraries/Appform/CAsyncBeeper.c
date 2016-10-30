/*/
    Copyright �1996, Juri Munkki
    All rights reserved.

    File: CAsyncBeeper.c
    Created: Tuesday, July 23, 1996, 2:30
    Modified: Tuesday, July 23, 1996, 2:55
/*/

#define	MAIN_ASYNCBEEPER
#include "CAsyncBeeper.h"

void	CAsyncBeeper::IAsyncBeeper()
{
	beepChannel = NULL;
	beepSound = NULL;
	gAsyncBeeper = this;
}

void	CAsyncBeeper::Dispose()
{
	if(beepChannel)
	{	SndDisposeChannel(beepChannel, true);
		beepChannel = NULL;
	}

	if(gAsyncBeeper == this)
	{	gAsyncBeeper = NULL;
	}
	
	inherited::Dispose();
}

void	CAsyncBeeper::BeepDoneCheck()
{
	SCStatus	soundStatus;

	if(beepChannel && noErr == SndChannelStatus(beepChannel, sizeof(SCStatus), &soundStatus))
	{	if(!soundStatus.scChannelBusy)
		{	SndDisposeChannel(beepChannel, true);
			beepChannel = NULL;

			if(beepSound)
			{	HUnlock(beepSound);
				ReleaseResource(beepSound);
			}
		}
	}
}

void	CAsyncBeeper::AsyncBeep(
	Handle	theSoundRes)
{
	if(beepChannel)
		BeepDoneCheck();
	
	if(beepChannel == NULL)
	{	OSErr	iErr;
		
		if(noErr == SndNewChannel(&beepChannel, 0, 0, NULL))
		{	HLock(theSoundRes);
			beepSound = theSoundRes;
			iErr = SndPlay(beepChannel, (SndListHandle)theSoundRes, true);
		}
	}
}

void	CAsyncBeeper::PlayNamedBeep(
	StringPtr	theName)
{
	if(beepChannel)
		BeepDoneCheck();
	
	if(beepChannel == NULL)
	{	Handle		theSound;
		
		theSound = GetNamedResource('snd ', theName);
		
		if(theSound)
		{	AsyncBeep(theSound);
		}
	}
}