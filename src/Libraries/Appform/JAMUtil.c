/*
    Copyright �1996, Juri Munkki
    All rights reserved.

    File: JAMUtil.c
    Created: Wednesday, April 24, 1996, 1:47
    Modified: Thursday, July 25, 1996, 3:08
*/

#include "JAMUtil.h"

short	GetSetTextSettings(
	TextSettings	*saveTo,
	short			newFont,
	short			newFace,
	short			newSize,
	short			newMode)
{
	FontInfo	theInfo;

	saveTo->savedFont = qd.thePort->txFont;
	saveTo->savedSize = qd.thePort->txSize;
	saveTo->savedFace = qd.thePort->txFace;
	saveTo->savedMode = qd.thePort->txMode;
	
	if(newFont >= 0)	TextFont(newFont);
	if(newSize >= 0)	TextSize(newSize);
	if(newFace >= 0)	TextFace(newFace);
	if(newSize != -1)	TextMode(newMode);
	
	GetFontInfo(&saveTo->newInfo);
	saveTo->nextLine = saveTo->newInfo.ascent + saveTo->newInfo.descent + saveTo->newInfo.leading;
	
	return saveTo->newInfo.ascent;
}

void	RestoreTextSettings(
	TextSettings *savedSettings)
{
	TextFont(savedSettings->savedFont);
	TextFace(savedSettings->savedFace);
	TextSize(savedSettings->savedSize);
	TextMode(savedSettings->savedMode);
}

short	GameTimeToString(
	long		theTime,
	StringPtr	theString)
{
	StringPtr	timeTo;
	long		minutes;
	long		seconds;

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
	
	return StringWidth(theString);
}
