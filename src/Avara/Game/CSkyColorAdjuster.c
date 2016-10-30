/*/
    Copyright �1994-1996, Juri Munkki
    All rights reserved.

    File: CSkyColorAdjuster.c
    Created: Tuesday, November 29, 1994, 8:24
    Modified: Friday, June 21, 1996, 18:08
/*/

#include "CSkyColorAdjuster.h"
#include "CWorldShader.h"

void	CSkyColorAdjuster::BeginScript()
{
	ProgramLongVar(iCount, 8);
	ProgramLongVar(iVerticalRangeMin, 0);
	ProgramLongVar(iVerticalRangeMax, 1000);
}

CAbstractActor *CSkyColorAdjuster::EndScript()
{
	long	shadeCount;
	Fixed	lowAlt, highAlt;
	
	CWorldShader	*theShader;

	inherited::EndScript();

	theShader = gCurrentGame->worldShader;
	theShader->lowSkyColor = GetPixelColor();
	theShader->highSkyColor = GetOtherPixelColor();

	shadeCount = ReadLongVar(iCount);
	if(shadeCount < 0) shadeCount = 0;
	else if(shadeCount > 32) shadeCount = 32;

	lowAlt = ReadFixedVar(iVerticalRangeMin);
	highAlt = ReadFixedVar(iVerticalRangeMax);

	theShader->lowSkyAltitude = lowAlt;
	theShader->highSkyAltitude = highAlt;
	theShader->skyShadeCount = shadeCount;

	Dispose();
	return NULL;
}
