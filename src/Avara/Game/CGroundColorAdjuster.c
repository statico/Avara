/*/
    Copyright �1994-1995, Juri Munkki
    All rights reserved.

    File: CGroundColorAdjuster.c
    Created: Tuesday, November 29, 1994, 8:24
    Modified: Saturday, December 30, 1995, 8:39
/*/

#include "CGroundColorAdjuster.h"
#include "CWorldShader.h"

CAbstractActor *CGroundColorAdjuster::EndScript()
{
	CWorldShader	*theShader;

	inherited::EndScript();

	theShader = gCurrentGame->worldShader;
	theShader->groundColor = GetPixelColor();

	Dispose();
	return NULL;
}
