/*/
    Copyright �1994-1995, Juri Munkki
    All rights reserved.

    File: CSmartPart.c
    Created: Wednesday, November 23, 1994, 0:43
    Modified: Sunday, December 10, 1995, 12:57
/*/

#include "CSmartPart.h"
#include "FastMat.h"

void	CSmartPart::ISmartPart(
	short			resId,
	CAbstractActor	*anActor,
	short			aPartCode)
{
	inherited::IBSPPart(resId);
	
	theOwner = anActor;
	partCode = aPartCode;
	
	rSquare[0] = 0;
	rSquare[1] = 0;
	FSquareAccumulate(header.enclosureRadius, rSquare);

	MoveDone();
}

Boolean	CSmartPart::HitTestBoxes(
register	CSmartPart *other)
{
	Vector	otherCorners[8];
	Vector	bupCorners[8];
	int		vCount;
	Vector	*v;
	Fixed	boundary;
	Matrix	combinedTransform;

	CombineTransforms(		&other->itsTransform,
							&combinedTransform,
							&invGlobTransform);

	TransformBoundingBox(&combinedTransform, 
						&other->header.minBounds.x,
						&other->header.maxBounds.x,
						otherCorners);

	boundary = header.minBounds.x;

	for(v = otherCorners, vCount = 8; (*v)[0] <= boundary; v++)
	{	if(--vCount == 0)
		{	return false;
		}
	}

	boundary = header.maxBounds.x;

	for(v = otherCorners, vCount = 8; (*v)[0] >= boundary; v++)
	{	if(--vCount == 0)
		{	return false;
		}
	}

	boundary = header.minBounds.y;

	for(v = otherCorners, vCount = 8; (*v)[1] <= boundary; v++)
	{	if(--vCount == 0)
		{	return false;
		}
	}

	boundary = header.maxBounds.y;

	for(v = otherCorners, vCount = 8; (*v)[1] >= boundary; v++)
	{	if(--vCount == 0)
		{	return false;
		}
	}

	boundary = header.minBounds.z;

	for(v = otherCorners, vCount = 8; (*v)[2] <= boundary; v++)
	{	if(--vCount == 0)
		{	return false;
		}
	}
	boundary = header.maxBounds.z;

	for(v = otherCorners, vCount = 8; (*v)[2] >= boundary; v++)
	{	if(--vCount == 0)
		{	return false;
		}
	}

	return true;
}

Boolean	CSmartPart::CollisionTest(
	CSmartPart	*other)
{
	register	Fixed		quickDist;
				Boolean		doCollide = false;
	
	quickDist = FDistanceEstimate(
					sphereGlobCenter[0] - other->sphereGlobCenter[0],
					sphereGlobCenter[1] - other->sphereGlobCenter[1],
					sphereGlobCenter[2] - other->sphereGlobCenter[2]);
	if(quickDist < header.enclosureRadius+other->header.enclosureRadius)
	{	//	It seems the bounding spheres overlap, so we have to do more exhaustive tests.
		//	We end up here about half the time this function is called, if reasonable initial
		//	culling is done.
		Vector	localSphere;
		long	acc[2];
	
		//	First, see if the bounding sphere of this one touches the bounding box of the
		//	other one:

		if(!other->invGlobDone)
		{	other->invGlobDone = true;
			InverseTransform(&other->itsTransform, &other->invGlobTransform);
		}

		VectorMatrix34Product(1, &sphereGlobCenter, &localSphere, &other->invGlobTransform);
		
		acc[0] = rSquare[0];	acc[1] = rSquare[1];

		quickDist = localSphere[0] - other->header.minBounds.x;
		if(quickDist < 0)
			FSquareSubtract(quickDist, acc);
		else
		{	quickDist = localSphere[0] - other->header.maxBounds.x;
			if(quickDist > 0)
				FSquareSubtract(quickDist, acc);
		}
		
		quickDist = localSphere[1] - other->header.minBounds.y;
		if(quickDist < 0)
			FSquareSubtract(quickDist, acc);
		else
		{	quickDist = localSphere[1] - other->header.maxBounds.y;
			if(quickDist > 0)
				FSquareSubtract(quickDist, acc);
		}

		quickDist = localSphere[2] - other->header.minBounds.z;
		if(quickDist < 0)
			FSquareSubtract(quickDist, acc);
		else
		{	quickDist = localSphere[2] - other->header.maxBounds.z;
			if(quickDist > 0)
				FSquareSubtract(quickDist, acc);
		}
		
		if(acc[0] >= 0)
		{
			//	Next, see if the bounding sphere of the other one touches the bounding box of
			//	this one:
	
			if(!invGlobDone)
			{	invGlobDone = true;
				InverseTransform(&itsTransform, &invGlobTransform);
			}
	
			VectorMatrix34Product(1, &other->sphereGlobCenter, &localSphere, &invGlobTransform);
			
			acc[0] = other->rSquare[0];	acc[1] = other->rSquare[1];
	
			quickDist = localSphere[0] - header.minBounds.x;
			if(quickDist < 0)
				FSquareSubtract(quickDist, acc);
			else
			{	quickDist = localSphere[0] - header.maxBounds.x;
				if(quickDist > 0)
					FSquareSubtract(quickDist, acc);
			}
			
			quickDist = localSphere[1] - header.minBounds.y;
			if(quickDist < 0)
				FSquareSubtract(quickDist, acc);
			else
			{	quickDist = localSphere[1] - header.maxBounds.y;
				if(quickDist > 0)
					FSquareSubtract(quickDist, acc);
			}
	
			quickDist = localSphere[2] - header.minBounds.z;
			if(quickDist < 0)
				FSquareSubtract(quickDist, acc);
			else
			{	quickDist = localSphere[2] - header.maxBounds.z;
				if(quickDist > 0)
					FSquareSubtract(quickDist, acc);
			}
			if(acc[0] >= 0)
			{	//	This happens only one time out of ten of the calls to this method.
			
				doCollide = HitTestBoxes(other);
		
				if(doCollide)
				{	doCollide = other->HitTestBoxes(this);
				}
			}
		}
	}
	
	return doCollide;
}

void	CSmartPart::RayTest(
register	RayHitRecord	*hitRec)
{
	Fixed	quickDist;
	Vector	deltas;
	
	deltas[0] = sphereGlobCenter[0] - hitRec->origin[0];
	deltas[1] = sphereGlobCenter[1] - hitRec->origin[1];
	deltas[2] = sphereGlobCenter[2] - hitRec->origin[2];
	
	//	First, check to see if the object is even close enough to qualify.
	quickDist = FDistanceEstimate(deltas[0], deltas[1], deltas[2]);
	
	if(quickDist - header.enclosureRadius < hitRec->distance)
	{	//	Now we know it's close enough. Let's do a real sphere intersect test.
		long	acc[2];
		Fixed	v;
		
		acc[0] = rSquare[0];
		acc[1] = rSquare[1];
		v = FMul(deltas[0], hitRec->direction[0])+
			FMul(deltas[1], hitRec->direction[1])+
			FMul(deltas[2], hitRec->direction[2]);

		FSquareSubtract(	deltas[0], acc);
		FSquareSubtract(	deltas[1], acc);
		FSquareSubtract(	deltas[2], acc);

		FSquareAccumulate(	v, acc);
		
		if(acc[0] >= 0)
		{	Fixed	sphereDistance;

			sphereDistance = v - FSqroot(acc);
			
			if(sphereDistance < hitRec->distance)			
			{	//	Ok, so the ray hits the bounding sphere and it's very close.
				//	Let's do a bounding box test and derive the distance from that.
				//	To do that, we need the ray in local coordinates for this box.
				//	This algorith is also from a graphics GEM

				#define NUMDIM	3
				#define RIGHT 0
				#define	LEFT 1
				#define	MIDDLE 2
				
				Fixed	*mins;
				Fixed	*maxs;
				Vector	newOrigin;
				Vector	newDirection;
				Boolean	inside = true;
				short	quadrant[NUMDIM];
				short	i;
				short	whichPlane;
				Fixed	maxT[NUMDIM];
				Fixed	candidate[NUMDIM];
				
				mins = &header.minBounds.x;
				maxs = &header.maxBounds.x;
				if(!invGlobDone)
				{	invGlobDone = true;
					InverseTransform(&itsTransform, &invGlobTransform);
				}
				
				VectorMatrix33Product(1, &hitRec->direction, &newDirection, &invGlobTransform);
				VectorMatrix34Product(1, &hitRec->origin, &newOrigin, &invGlobTransform);

				for(i=0;i<NUMDIM;i++)
				{	if(newOrigin[i] <= mins[i])
					{	quadrant[i] = LEFT;
						candidate[i] = mins[i];
						inside = false;
					}
					else
					if(newOrigin[i] >= maxs[i])
					{	quadrant[i] = RIGHT;
						candidate[i] = maxs[i];
						inside = false;
					}
					else
						quadrant[i] = MIDDLE;
				}
				
				if(inside)
				{	if(sphereDistance > 0)
					{	hitRec->distance = sphereDistance;
						hitRec->closestHit = this;
					}
				}
				else
				{	Fixed	boxDist;
					short	whichPlane = 0;
					
					for(i=0;i<NUMDIM;i++)
					{	if(quadrant[i] != MIDDLE && newDirection[i])
							maxT[i] = FDivNZ(candidate[i]-newOrigin[i], newDirection[i]);
						else
							maxT[i] = FIX(-1);
					}
					
					boxDist = maxT[0];
					for(i=1;i<NUMDIM;i++)
					{	if(boxDist < maxT[i])
						{	boxDist = maxT[i];
							whichPlane = i;
						}
					}

					if(boxDist >= 0 && boxDist < hitRec->distance)
					{	Fixed	coord;
						for(i=0;i<NUMDIM;i++)
						{	if(i != whichPlane)
							{	coord = newOrigin[i] + FMul(boxDist, newDirection[i]);
								if(coord > maxs[i] || coord < mins[i])
								{	return;
								}
							}
						}
						if(boxDist > sphereDistance)
							hitRec->distance = boxDist;
						else
							hitRec->distance = sphereDistance;

						hitRec->closestHit = this;
					}
				}
			}
		}
	}	
}

Boolean		CSmartPart::IsInRange(
	Fixed	*origin,
	Fixed	range)
{
	Fixed	distEstimate;
	
	distEstimate = FDistanceEstimate(
					sphereGlobCenter[0] - origin[0],
					sphereGlobCenter[1] - origin[1],
					sphereGlobCenter[2] - origin[2]);
	return range + header.enclosureRadius > distEstimate;
}

void	CSmartPart::MinMaxGlobalBox(
	Fixed	*minPoint,
	Fixed	*maxPoint)
{
	Fixed	*p,*q;
	short	i,j;
	Vector	boxGlobCorners[8];
	
	TransformBoundingBox(&itsTransform, 
						&header.minBounds.x,
						&header.maxBounds.x,
						boxGlobCorners);

	p = &boxGlobCorners[0][0];

	i = 3;
	do
	{	Fixed	min,max;
		Fixed	val;

		q = p++;

		val = *q;
		q += 4;

		min = *minPoint;
		if(val < min)	min = val;
		max = *maxPoint;
		if(val > max)	max = val;

		j = 7;
		do
		{	val = *q;
			q += 4;
			if(val < min)		min = val;
			else if(val > max)	max = val;
		} while(--j);
		
		*minPoint++ = min;
		*maxPoint++ = max;
	} while(--i);
}

void	CSmartPart::MinMaxGlobalSphere(
register	Fixed	*minPoint,
register	Fixed	*maxPoint)
{
	Fixed	val;
	Fixed	*v;
	short	i;
	
	v = sphereGlobCenter;
	i = 3;
	do
	{
		val = *v - header.enclosureRadius;
		if(val < *minPoint)	*minPoint = val;
		minPoint++;
		
		val = *v++ + header.enclosureRadius;
		if(val > *maxPoint)	*maxPoint = val;
		maxPoint++;

	} while(--i);
}

void	CSmartPart::OffsetPart(
	Fixed	*offset)
{
	Fixed	x,y,z;
	short	i;
	
	x = offset[0];
	y = offset[1];
	z = offset[2];
	
	invGlobDone = false;
	itsTransform[3][0] += x;
	itsTransform[3][1] += y;
	itsTransform[3][2] += z;

	sphereGlobCenter[0] += x;
	sphereGlobCenter[1] += y;
	sphereGlobCenter[2] += z;
}