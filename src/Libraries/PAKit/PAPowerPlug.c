/*/
    Copyright �1995-1996, Juri Munkki
    All rights reserved.

    File: PAPowerPlug.c
    Created: Monday, May 1, 1995, 8:10
    Modified: Saturday, February 10, 1996, 18:52
/*/

#include "PAKit.h"

/*
void		ScanConvertPolysC(	short			*destBuffer,
								PolyEdgePtr		*thisOn,
								PolyEdgePtr		*thisOff,
								Rect			*theBounds,
								short			bgColor);

//#define ScanConvertPolys ScanConvertPolysC
*/

void	ScanConvertPolysC(
	PolyWorld		*polys,
	short			*destBuffer,
	PolyEdgePtr		*thisOn,
	PolyEdgePtr		*thisOff,
	Rect			*theBounds,
	short			bgColor)
{
	short			*emergencyStop;
	PolyEdgePtr		activeListArray[ACTIVELISTSIZE];
	PolyEdgePtr		coherenceCache[ACTIVELISTSIZE];
	PolyEdgePtr		*coherencePtr;
	PolyEdgePtr		*activeList;
	PolyEdgePtr		*lastActive;
	PolyEdge		backgroundColor,sentinel, rightSentinel;
	short			i;
	Rect			bounds;
	short			rightEdge;
	
	emergencyStop = destBuffer+polys->displayRegionSize-2*ACTIVELISTSIZE;

	bounds = *theBounds;
	rightEdge = bounds.right;

	i = bounds.bottom - bounds.top;
	
	backgroundColor.color = bgColor;
	backgroundColor.polyID = 0;
	backgroundColor.x = (long)bounds.left << 16;

	*(short *)&rightSentinel.x = rightEdge;
	sentinel.x = 0x80000000L;
	activeList = activeListArray;
	*activeList++ = &sentinel;
	lastActive = activeList;
	
	coherencePtr = NULL;

	do
	{
		{	PolyEdgePtr	newEdge;
	
			newEdge = *thisOn;
			*thisOn++ = NULL;
			
			if(newEdge)
			{
				coherencePtr = NULL;
	
				do
				{	*lastActive++ = newEdge;
					newEdge = (PolyEdgePtr)newEdge->nextOn;
				} while(newEdge);
			}
		}
		//	Insertion sort active list:
		if(activeList < lastActive)
		{	PolyEdgePtr		*insert;
			PolyEdgePtr		*prev;
			PolyEdgePtr		item;
			PolyEdgePtr		comp;
			short			key;
		
			insert = activeList;
			do
			{
				prev = insert;
				item = *insert++;
				item->x += item->dx;
				key = *(short *)(&item->x);
				
				while(	comp = *--prev,
						(*(short *)(&comp->x) > key))
				{	//	Swap items.
					prev[1] = comp;
					prev[0] = item;
					coherencePtr = NULL;
				}
				
			} while(insert < lastActive);
		}

		if(lastActive > activeList)
		{	short		topPolyID;
			short		topColor;
			PolyEdgePtr	*edgeList;
			PolyEdgePtr	stackTop;
			
			PolyEdgePtr	tempTop;
			PolyEdgePtr	oldPosition;
			
			PolyEdgePtr	edge;
			short		theId;
			short		currentX;
			short		newX;
				
			topColor = bgColor;
			currentX = bounds.left;		

			if(coherencePtr)
			{	
				edgeList = coherenceCache;
				edge = *edgeList++;
				newX = *(short *)(&edge->x);							

				while(newX < rightEdge)
				{	if(newX > currentX)
					{	currentX = newX;
						*destBuffer++ = topColor;
						*destBuffer++ = currentX;
					}
				
					topColor = edge->color;
					edge = *edgeList++;
					newX = *(short *)(&edge->x);
				}	
			}
			else
			{
				coherencePtr = coherenceCache;
				stackTop = &backgroundColor;
				topPolyID = 0;
	
				edgeList = activeList;
				
				do
				{	edge = *edgeList++;
					theId = edge->polyID;
					
					if(theId < topPolyID)
					{
#if 1
						tempTop = (PolyEdgePtr)stackTop->nextOn;
						oldPosition = stackTop;
						
						while(tempTop->polyID > theId)
						{	oldPosition = tempTop;
							tempTop = (PolyEdgePtr)tempTop->nextOn;
						}
						
						if(tempTop->polyID < theId)
						{	oldPosition->nextOn = edge;
							edge->nextOn = tempTop;
						}
						else
						{	oldPosition->nextOn = tempTop->nextOn;
						}
#else
						do
						{	oldPosition = tempTop;
							tempTop = (PolyEdgePtr)tempTop->nextOn;
							if(tempTop->polyID < theId)
							{	oldPosition->nextOn = edge;
								edge->nextOn = tempTop;
								break;
							}
							else
							if(tempTop->polyID == theId)
							{	oldPosition->nextOn = tempTop->nextOn;
								break;
							}
						} while(1);
#endif
					}
					else
					{	if(theId == topPolyID)
						{	stackTop = (PolyEdgePtr)stackTop->nextOn;
							*coherencePtr++ = edge;
						}
						else
						{	edge->nextOn = stackTop;
							stackTop = edge;
						}
						*coherencePtr++ = stackTop;
						newX = *(short *)(&edge->x);
						
						if(newX < rightEdge)
						{	if(newX > currentX)
							{	currentX = newX;
								*destBuffer++ = topColor;
								*destBuffer++ = currentX;
							}
						
							topColor = stackTop->color;
						}
	
						topPolyID = stackTop->polyID;
					}
				} while(edgeList < lastActive);

				*coherencePtr++ = &rightSentinel;
			}

			*destBuffer++ = topColor;
			*destBuffer++ = rightEdge;
		}
		else
		{
			*destBuffer++ = bgColor;
			*destBuffer++ = rightEdge;
		}

		
		{	PolyEdgePtr	deadEdge;
			PolyEdgePtr	*newList;
			PolyEdgePtr	*oldList;

			deadEdge = *thisOff;
			*thisOff++ = NULL;
	
			if(deadEdge)
			{
				coherencePtr = NULL;
				do
				{	deadEdge->polyID = 0;
					deadEdge = (PolyEdgePtr)deadEdge->nextOff;
				} while(deadEdge);
				
				newList = activeList;
				oldList = newList;
				do
				{	if((*oldList)->polyID)
					{	*newList++ = *oldList;
					}
				} while(++oldList < lastActive);
				
				lastActive = newList;
			}
		}
		
		if(emergencyStop < destBuffer)
		{	while(--i)
			{	*thisOn++ = NULL;
				*thisOff++ = NULL;
				*destBuffer++ = bgColor;
				*destBuffer++ = rightEdge;
			}
			i = 1;
		}
	} while(--i);
}

#if	0
void	ScanConvertPolysCTidy(
	PolyWorld		*polys,
	short			*destBuffer,
	PolyEdgePtr		*thisOn,
	PolyEdgePtr		*thisOff,
	Rect			*theBounds,
	short			bgColor)
{
	short			*emergencyStop;
	PolyEdgePtr		activeListArray[ACTIVELISTSIZE];
	PolyEdgePtr		*activeList;
	PolyEdgePtr		*lastActive;
	PolyEdge		backgroundColor,sentinel;
	short			i;
	Rect			bounds;
	short			rightEdge;
	
	emergencyStop = destBuffer+polys->displayRegionSize-2*ACTIVELISTSIZE;

	bounds = *theBounds;
	rightEdge = bounds.right;

	i = bounds.bottom - bounds.top;
	
	backgroundColor.color = bgColor;
	backgroundColor.polyID = 0;
	backgroundColor.x = (long)bounds.left << 16;

	sentinel.x = 0x80000000L;
	activeList = activeListArray;
	*activeList++ = &sentinel;
	lastActive = activeList;

	do
	{
		//	Append new edge records into active list:
		{	PolyEdgePtr	newEdge;
	
			newEdge = *thisOn;
			*thisOn++ = NULL;
	
			while(newEdge)
			{	*lastActive++ = newEdge;
				newEdge = (PolyEdgePtr)newEdge->nextOn;
			}
		}
		
		//	Insertion sort active list:
		if(activeList < lastActive)
		{	PolyEdgePtr		*insert;
			PolyEdgePtr		*prev;
			PolyEdgePtr		item;
			PolyEdgePtr		comp;
			short			key;
		
			insert = activeList;
			do
			{
				prev = insert;
				item = *insert++;
				item->x += item->dx;
				key = *(short *)(&item->x);
				
				while(	comp = *--prev,
						(*(short *)(&comp->x) > key))
				{	//	Swap items.
					prev[1] = comp;
					prev[0] = item;
				}
				
			} while(insert < lastActive);
		}

		if(lastActive > activeList)
		{	short		leftClip;
			short		topPolyID;
			short		topColor;
			PolyEdgePtr	*edgeList;
			PolyEdgePtr	stackTop;
			
			PolyEdgePtr	tempTop;
			PolyEdgePtr	oldPosition;
			
			PolyEdgePtr	edge;
			short		theId;
			short		currentX;
			short		newX;
				
			topColor = bgColor;

			stackTop = &backgroundColor;
			currentX = bounds.left;
			topPolyID = 0;

			edgeList = activeList;
			
			do
			{	edge = *edgeList++;
				theId = edge->polyID;
				
				if(theId < topPolyID)
				{	tempTop = stackTop;
					
					do
					{	oldPosition = tempTop;
						tempTop = (PolyEdgePtr)tempTop->nextOn;
						if(tempTop->polyID < theId)
						{	oldPosition->nextOn = edge;
							edge->nextOn = tempTop;
							break;
						}
						else
						if(tempTop->polyID == theId)
						{	oldPosition->nextOn = tempTop->nextOn;
							break;
						}
					} while(1);
				}
				else
				{	if(theId == topPolyID)
					{	stackTop = (PolyEdgePtr)stackTop->nextOn;
					}
					else
					{	edge->nextOn = stackTop;
						stackTop = edge;
					}
					
					newX = *(short *)(&edge->x);
					
					if(newX < rightEdge)
					{	if(newX > currentX)
						{	currentX = newX;
							*destBuffer++ = topColor;
							*destBuffer++ = currentX;
						}
					
						topPolyID = stackTop->polyID;
						topColor = stackTop->color;
					}
				}
			} while(edgeList < lastActive);

			*destBuffer++ = topColor;
			*destBuffer++ = rightEdge;
		}
		else
		{
			*destBuffer++ = bgColor;
			*destBuffer++ = rightEdge;
		}
		
		{	PolyEdgePtr	deadEdge;
			PolyEdgePtr	*newList;
			PolyEdgePtr	*oldList;
	
			deadEdge = *thisOff;
			*thisOff++ = NULL;
	
			if(deadEdge)
			{	do
				{	deadEdge->polyID = 0;
					deadEdge = (PolyEdgePtr)deadEdge->nextOff;
				} while(deadEdge);
				
				newList = activeList;
				oldList = newList;
				do
				{	if((*oldList)->polyID)
					{	*newList++ = *oldList;
					}
				} while(++oldList < lastActive);
				
				lastActive = newList;
			}
		}

		if(emergencyStop < destBuffer)
		{	while(--i)
			{	*thisOn++ = NULL;
				*thisOff++ = NULL;
				*destBuffer++ = bgColor;
				*destBuffer++ = rightEdge;
			}
			i = 1;
		}
	} while(--i);
}
#endif

void	DiffPolyRegionsC(
	PolyWorld	*polys,
	short		*oldScreen,
	short		*newScreen,
	short		*diffScreen,
	short		rowCount)
{
	short	*emergencyStop;
	short	oldColor, newColor;
	short	oldX, newX;
	short	currentColor;
	short	currentX;
	short	leftEdge = polys->bounds.left;
	short	rightEdge = polys->bounds.right;

	emergencyStop = diffScreen + polys->deltaRegionSize - rowCount - 2*ACTIVELISTSIZE;

	while(rowCount--)
	{	
		currentColor = -1;
		currentX = leftEdge;
	
		do
		{	newColor = *newScreen++;
			newX = *newScreen++;
			
			oldColor = *oldScreen++;
			oldX = *oldScreen++;
			
		mainLoop:
			if(oldColor == newColor)
			{	if(currentColor < 0) goto readMore;
				*diffScreen++ = currentX;
				*diffScreen++ = currentColor = -1;
				
				if(oldX == newX) goto readBoth;
				if(oldX < newX) goto readOld;
				goto readNew;
			}
			else
			{	if(newColor == currentColor) goto readMore;
				*diffScreen++ = currentX;
				*diffScreen++ = currentColor = newColor;
			}
		readMore:
			if(oldX == newX) goto readBoth;
			if(oldX < newX) goto readOld;
		readNew:
			currentX = newX;
			newColor = *newScreen++;
			newX = *newScreen++;
			goto mainLoop;

		readOld:
			currentX = oldX;
			oldColor = *oldScreen++;
			oldX = *oldScreen++;
			goto mainLoop;
		readBoth:
			currentX = newX;
						
		} while(currentX < rightEdge);

		*diffScreen++ = currentX;
		if(diffScreen > emergencyStop)
		{	while(rowCount--)
			{	*diffScreen++ = currentX;
			}
			
			rowCount = 0;
		}
	}
}

typedef long	bytePat[2];
void	BytePixelDriverC(
	short		*diffp,
	void		*colorTable,
	short		rowBytes,
	void		*baseAddr,
	short		rowCount,
	short		rightEdge)
{
	bytePat	*colors;
	short	theColor;
	long	pixels, pixels2;
	short	x1;
	char	*addr;
	char	*dest;
	char	*endAddr;
	long	patOffset = sizeof(long);
	
	colors = *(bytePat **)colorTable;
	addr = (char *)baseAddr;
	
	while(rowCount--)
	{	x1 = *diffp++;
		while(x1 < rightEdge)
		{
			theColor = *diffp++;

			if(theColor < 0)
			{	x1 = *diffp++;
			}
			else
			{	long	judge;
				long	delta;

				pixels = colors[theColor][0];
				pixels2 = pixels>>8;
				delta = x1;
				dest = addr + x1;
				x1 = *diffp++;
				endAddr = addr + x1;

				judge = 3 & (long)dest;
				if(judge)
				{	if(judge == 1)
					{	*dest++ = pixels;
						if(dest >= endAddr)	goto done;
						*dest++ = pixels2;
						if(dest >= endAddr)	goto done;
						*dest++ = pixels;
						if(dest >= endAddr)	goto done;
					}
					else
					if(judge == 2)
					{	*dest++ = pixels2;
						if(dest >= endAddr)	goto done;
						*dest++ = pixels;
						if(dest >= endAddr)	goto done;
					}
					else
					{	*dest++ = pixels;
						if(dest >= endAddr)	goto done;
					}
				}

				endAddr -= 4;
				while(dest <= endAddr)
				{	*(long *)dest = pixels;
					dest += 4;					
				}
				endAddr += 4;

				judge = 3 & (long)endAddr;
				if(judge)
				{	if(judge == 3)
					{	*(short *)dest = pixels;
						dest[2] = pixels2;
					}
					else
					if(judge == 2)
					{	*(short *)dest = pixels;
					}
					else
					{	dest[0] = pixels2;
					}
				}
		done:;
			}
		}
		
		colors = (bytePat *)(patOffset + (char *)colors);
		addr += rowBytes;
		patOffset = -patOffset;
	}
}

void	True16PixelDriverC(
	short		*diffp,
	void		*colorTable,
	short		rowBytes,
	void		*baseAddr,
	short		rowCount,
	short		rightEdge)
{
	long	*colors;
	short	theColor;
	long	pixels;
	short	x1;
	char	*addr;
	short	*dest;
	short	*endAddr;
	
	colors = *(long **)colorTable;
	addr = (char *)baseAddr;
	
	while(rowCount--)
	{	x1 = *diffp++;
		while(x1 < rightEdge)
		{
			theColor = *diffp++;

			if(theColor < 0)
			{	x1 = *diffp++;
			}
			else
			{
				pixels = colors[theColor];
				dest = x1 + (short *)addr;
				x1 = *diffp++;
				endAddr = x1 + (short *)addr;

				if(2 & (long)dest)
				{	*dest++ = pixels;
					if(dest >= endAddr)	goto done;
				}
				
				endAddr -= 2;
				while(dest <= endAddr)
				{	*(long *)dest = pixels;
					dest += 2;
				}
				endAddr += 2;

				while(dest < endAddr)
				{	*dest++ = pixels;
				}
		done:;
			}
		}
		
		addr += rowBytes;

	}
}

void	True24PixelDriverC(
	short		*diffp,
	void		*colorTable,
	short		rowBytes,
	void		*baseAddr,
	short		rowCount,
	short		rightEdge)
{
	long	*colors;
	short	theColor;
	long	pixels;
	short	x1;
	char	*addr;
	long	*dest;
	long	*endAddr;
	
	colors = *(long **)colorTable;
	addr = (char *)baseAddr;
	
	while(rowCount--)
	{	x1 = *diffp++;
		while(x1 < rightEdge)
		{
			theColor = *diffp++;

			if(theColor < 0)
			{	x1 = *diffp++;
			}
			else
			{
				pixels = colors[theColor];
				dest = x1 + (long *)addr;
				x1 = *diffp++;
				endAddr = x1 + (long *)addr;

				while(dest < endAddr)
				{	*dest++ = pixels;
				}
			}
		}
		
		addr += rowBytes;

	}
}

extern	PolyWorld	*thePolyWorld;
typedef	float		lineSlopeType;

lineSlopeType	doubleOne = 65536.0;

PolyEdgePtr	AddFPolySegmentC(
	Fixed	ax,
	Fixed	ay,
	Fixed	bx,
	Fixed	by)
{
	Fixed			rightBound;
	Fixed			xOffset;
	Fixed			deltaY;
	long			aly, bly;
	lineSlopeType	slope;
	PolyWorld		*polys;
	PolyEdgePtr		edge = NULL;
	
	polys = thePolyWorld;
	rightBound = ((long)polys->bounds.right) << 16;
	xOffset = ((long)polys->xOffset) << 16;
	
	ax += xOffset;
	bx += xOffset;

	if(ax < rightBound || bx < rightBound)
	{	if(ay > by)
		{	Fixed	temp;
			
			temp = ay;	ay = by;	by = temp;
			temp = ax;	ax = bx;	bx = temp;
		}
		
		deltaY = by - ay;
		
		aly = ((ay - 1) >> 16) + 1;
		if(aly >= polys->height)	goto noEdge;

		bly = ((by - 1) >> 16) + 1;
		if(bly <= 0) 				goto noEdge;
		if(aly == bly)				goto noEdge;
		
		edge = polys->newEdge;
		if(edge >= polys->endEdge)
		{	edge = NULL;
			goto noEdge;
		}
		
		slope = (bx - ax) / ((lineSlopeType) deltaY);
		edge->dx = slope * doubleOne;
		
		if(aly >= 0)
		{	ax += slope * (unsigned short) -ay;
		}
		else
		{	aly = 0;
			ax -= slope * ay;
		}
		
		edge->x = ax - edge->dx;
		edge->color = polys->currentColor;
		edge->polyID = polys->polyCount;

		edge->nextOn = polys->onLists[aly];
		polys->onLists[aly] = edge;
		
		if(bly < polys->height)
		{	edge->nextOff = polys->offLists[bly-1];
			polys->offLists[bly-1] = edge;
		}
		
		polys->newEdge++;
	}

noEdge:
	return edge;
}

void	ReplicatePolySegmentC(
	PolyEdgePtr		theEdge)
{
	PolyWorld	*polys;
	PolyEdgePtr	newEdge;
	
	polys = thePolyWorld;
	if(theEdge && polys->newEdge < polys->endEdge)
	{	newEdge = polys->newEdge++;
		newEdge->x = theEdge->x;
		newEdge->dx = theEdge->dx;
		newEdge->polyID = polys->polyCount;
		newEdge->color = polys->currentColor;
		
		newEdge->nextOn = theEdge->nextOn;
		newEdge->nextOff = theEdge->nextOff;
		theEdge->nextOn = (PolyEdgePtr) newEdge;
		theEdge->nextOff = (PolyEdgePtr) newEdge;
	}
}