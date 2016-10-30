/*
    Copyright �1992-1995, Juri Munkki
    All rights reserved.

    File: PolyColor.h
    Created: Sunday, December 6, 1992, 6:26
    Modified: Tuesday, May 23, 1995, 18:00
*/

#pragma once

#define POLYCOLORCLUMPSIZE 16

typedef struct
{
	long			color;		//	Color in 24 bit format rgb.
	short			priority;	//	How badly do we need this color?
	unsigned short	nextColor;	//	Index to next color in list.
}	PolyColorEntry;

typedef struct
{
	long			seed;
	unsigned short	hashTable[64];
	unsigned short	polyColorCount;

	PolyColorEntry	**colorEntryHandle;
	long			realColorEntryHandleSize;
	long			logicalColorEntryHandleSize;
}	PolyColorTable;

void			InitPolyColorTable(PolyColorTable *);
void			ClosePolyColorTable(PolyColorTable *);
void			SetPolyColorTable(PolyColorTable *);
PolyColorTable	*GetPolyColorTable();

unsigned short	FindPolyColor(long theColor);
void			IncreasePolyColorPriority(unsigned short index, short increase);

Handle			ConvertToColorTable();

long			RGBToLongColor(RGBColor *theColor);