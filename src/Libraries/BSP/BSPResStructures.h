/*
    Copyright �1994, Juri Munkki
    All rights reserved.

    File: BSPResStructures.h
    Created: Wednesday, June 29, 1994, 23:31
    Modified: Sunday, December 11, 1994, 1:44
*/

#pragma once

#include "BSPConfig.h"

#define	BSPRESTYPE	'BSPT'

enum	{	clockWiseFace, counterClockFace, doubleSidedFace	};
enum	{	frontVisible = 1, backVisible, bothVisible			};

typedef	struct
{
	Fixed	x;
	Fixed	y;
	Fixed	z;
	Fixed	w;
} UniquePoint;

typedef struct
{
	short	a;
	short	b;
} EdgeRecord;

typedef struct
{
	short	normalIndex;		//	Index to normal vector list
	short	basePointIndex;		//	Index to point list.
	short	colorIndex;			//	Color of surfaces on this plane.
	short	visibilityFlags;

} NormalRecord;

typedef struct
{
	short		firstEdge;
	short		edgeCount;
	short		normalIndex;
	
	short		frontPoly;
	short		backPoly;

	short		visibility;
	long		reservedForFutureUse2;

} PolyRecord;

typedef short	EdgeIndex;

typedef struct
{
	long	theColor;
	short	colorCache[COLORCACHESIZE];
} ColorRecord;

typedef	struct
{
	short		refCount;
	short		lockCount;

	UniquePoint	enclosurePoint;
	Fixed		enclosureRadius;
	
	UniquePoint	minBounds;	//	Bounding box minimums for x, y, z
	UniquePoint	maxBounds;	//	Bounding box maximums for x, y, z

	long	normalCount;
	long	edgeCount;
	long	polyCount;
	long	colorCount;
	long	pointCount;
	long	vectorCount;
	long	uniqueEdgeCount;

	long	normalOffset;
	long	polyOffset;
	long	edgeOffset;
	long	colorOffset;
	long	pointOffset;
	long	vectorOffset;
	long	uniqueEdgeOffset;
} BSPResourceHeader;
