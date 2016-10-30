/*/
    Copyright �1994-1995, Juri Munkki
    All rights reserved.

    File: CHuffmanHandler.c
    Created: Thursday, December 22, 1994, 3:54
    Modified: Thursday, August 31, 1995, 20:29
/*/

#include "CHuffmanHandler.h"

#ifndef INTEL_ARCH
#define	MEM_LONG(val)	val
#define	MEM_SHORT(val)	val
#ifdef THINK_C
#define	INLINE_ASM
#endif
#else
#define	MEM_LONG(val)	((val << 24) | ((val << 8) & 0xFF0000) | ((val >> 8) & 0xFF00) | ((val >> 24) & 0xFF))
#define	MEM_SHORT(val)	(short)((val << 8) | ((val >> 8) & 0xFF))
#endif

/*
**	Decoding the data is a bit more complicated than writing it, since
**	we have to walk the tree to find what maps to what. We can however
**	take a shortcut by reading HUFFHANDLELOOKUPBITS bits at a time. If the
**	code we reach is shorter than this, all we need to do is back up
**	a few bits. If we did not reach a leaf node with the lookup, we
**	continue bit by bit until we reach a leaf.
**
**	This routine creates the lookup table for HUFFHANDLELOOKUPBITS bits and
**	it creates a table of lengths so that we can find out how many bits
**	were needed for the last char.
**
**	Recursion has been eliminated with a stack and the routine is a
**	modified version of the one used to create the tables in the encoder
**	object.
**
**	The routine goes through the patterns in order starting with 000... and ending
**	at 111... Because of this, the "holes" in the lookup table can be filled
**	as we go. The last pointer written to the table has to be replicated until
**	the end of the table.
**
**	If BuildTree returns a leaf node, then the tree can only have one symbol
**	in it and using huffman compression doesn't make any sense (since the
**	code consists of only one repeated symbol).
*/
void	CHuffmanHandler::CreateLookupBuffer()
{
	short			lastLookup;
	short			walkerStack[33];
	short			*stackPtr;
	short			codeLen = 1;
	long			codeString = 0;
	HuffTreeNode	*theNode;
	HuffTreeNode	*lastNode;

	theNode = BuildTree();

	singleSymbolData = (theNode->left < 0);
	theSingleSymbol = theNode->right;

	if(!singleSymbolData)	//	Must be branch node.
	{
		stackPtr = walkerStack+1;
		
		*stackPtr++ = theNode->right;
		theNode = &nodes[theNode->left];
		lastLookup = 0;
	
		do
		{	if(codeLen == HUFFHANDLELOOKUPBITS || (theNode->left < 0 && codeLen < HUFFHANDLELOOKUPBITS))
			{	long			codeIndex;
				short			i;
				
				codeIndex = codeString << (HUFFHANDLELOOKUPBITS - codeLen);
	
				lastNode = lookupBuf[lastLookup];
	
				while(lastLookup < codeIndex)
				{	lookupBuf[lastLookup++] = lastNode;
				}
				
				lookupBuf[codeIndex] = theNode;
							
			}
			
			if(theNode->left < 0)
			{	codeLengths[theNode->right] = codeLen;
	
				theNode = &nodes[*--stackPtr];
	
				while(codeString & 1)
				{	codeLen--;
					codeString >>= 1;
				};
	
				codeString |= 1;		
			}
			else
			{	*stackPtr++ = theNode->right;
				theNode = &nodes[theNode->left];
	
				codeLen++;
				codeString += codeString;
			}
		}
		while(stackPtr != walkerStack);
		
		lastNode = lookupBuf[lastLookup];
		while(lastLookup < HUFFHANDLELOOKUPSIZE)
		{	lookupBuf[lastLookup++] = lastNode;
		}
	}
}

/*
**	Create a mapping from characters to bit patterns.
**	Lengths are stored in a different array. Bit patterns
**	may be up to 24 bits long (the limitation comes from
**	the output code).
**
**	A stack is used to eliminate recursion while walking the tree.
**
**	If BuildTree returns a leaf node, then the tree can only have one symbol
**	in it and using huffman compression doesn't make any sense (since the
**	code consists of only one repeated symbol).
*/
void	CHuffmanHandler::CreateSymbolTable()
{
	short			walkerStack[33];
	short			*stackPtr;
	short			codeLen = 1;
	long			codeString = 0;
	HuffTreeNode	*theNode;

	theNode = BuildTree();
	
	singleSymbolData = (theNode->left < 0);

	if(!singleSymbolData)	//	Must be branch node.
	{
		stackPtr = walkerStack+1;
		
		*stackPtr++ = theNode->right;
		theNode = &nodes[theNode->left];
	
		do
		{	if(theNode->left < 0)
			{	codeStrings[theNode->right] = codeString;
				codeLengths[theNode->right] = codeLen;
				
				theNode = &nodes[*--stackPtr];
	
				while(codeString & 1)
				{	codeLen--;
					codeString >>= 1;
				};
	
				codeString |= 1;
			}
			else
			{	*stackPtr++ = theNode->right;
				theNode = &nodes[theNode->left];
	
				codeLen++;
				codeString += codeString;
			}
		}
		while(stackPtr != walkerStack);
	}
}

void	CHuffmanHandler::WriteCompressed()
{
	short				i,j,k;
	long				outputLen;
	long				outputBits = 0;
	short				nonZero = 0;
	HuffHandleHeader	*outHeader;
	short				countDataSize;
	
	if(singleSymbolData)
	{	outputBits = 0;
		nonZero = 1;
	}
	else
	{	//	Count how many bits are needed for output.
		for(i = 0;i<NUMSYMBOLS;i++)
		{	if(symbCounters[i])
			{	nonZero++;
				outputBits += symbCounters[i] * codeLengths[i];
			}
		}
	}
	
	countDataSize = dataCount > 65535 ? sizeof(long) : sizeof(short);

	outputLen = nonZero * countDataSize +	//	Stored counts
				sizeof(HuffHandleHeader) +	//	Header size
				((outputBits + 7) >> 3);	//	Number of encoded bytes

#ifdef INLINE_ASM
	outHandle = NewHandle(outputLen);
#else
	outHandle = NewHandleClear(outputLen);
#endif

	if(outHandle)
	{
		unsigned char			*inData, *endData;
		register long			outBits;
		register long			outLen;
		register long			bitOffset;
		register unsigned long	*countTable;
		register unsigned short	*countTableShort;

		outHeader = (HuffHandleHeader *)*outHandle;
		outHeader->decodedSize = MEM_LONG(dataCount);
		
		k = 0;
		for(i = 0;i < (NUMSYMBOLS>>5); i++)
		{	long	flags;
			
			for(j=0 ; j<32 ; j++)
			{	flags += flags;
				flags |= (symbCounters[k++] != 0);
			}
			
			outHeader->countBitmap[i] = MEM_LONG(flags);
		}	

		if(dataCount > 65535)
		{	countTable = (unsigned long *)(sizeof(HuffHandleHeader) + *outHandle);
			for(i = 0;i < NUMSYMBOLS; i++)
			{	if(symbCounters[i])
				{	*countTable++ = MEM_LONG(symbCounters[i]);
				}
			}
		}
		else
		{	countTableShort = (unsigned short *)(sizeof(HuffHandleHeader) + *outHandle);
			for(i = 0;i < NUMSYMBOLS; i++)
			{	if(symbCounters[i])
				{	*countTableShort++ = MEM_SHORT(symbCounters[i]);
				}
			}
			
			countTable = (unsigned long *) countTableShort;
		}
			
		if(!singleSymbolData)
		{	bitOffset = 0;
			inData = (unsigned char *)*inHandle;
			endData = inData + dataCount;
			
			while(inData < endData)
			{	long	*tempBitP;
	
				outBits = codeStrings[*inData];
				outLen = codeLengths[*inData++];
	
	#ifdef INLINE_ASM
				asm
				{	bfins	outBits,(countTable){bitOffset:outLen}
					add.l	outLen,bitOffset
				}
	#else
				tempBitP = (long *)((char *)countTable + (bitOffset >> 3));
	#ifdef INTEL_ARCH
				{	unsigned char	*destP = (unsigned char*)tempBitP;
					unsigned long	orData;
				
					orData = outBits << (32 - ((bitOffset & 7) + outLen));
					*destP++ |= orData >> 24;
					*destP++ |= orData >> 16;
					*destP++ |= orData >> 8;
					*destP |= orData;
				}
	#else
				*tempBitP |= outBits << (32 - ((bitOffset & 7) + outLen));	
	#endif
				bitOffset += outLen;
	#endif
			}
		}
	}
}

Handle	CHuffmanHandler::Compress(
	Handle	sourceHandle)
{
	register	long			i;
	register	unsigned char	*p;
	register	long			*t;
				long			stringsArray[NUMSYMBOLS];
				short			lengthsArray[NUMSYMBOLS];

	LockThis();
	codeStrings = stringsArray;
	codeLengths = lengthsArray;

	inHandle = sourceHandle;
	dataCount = GetHandleSize(inHandle);
	
	if(dataCount)
	{	t = symbCounters;
		for(i=0;i<NUMSYMBOLS;i++)
		{	t[i] = 0;
		}
			
		i = GetHandleSize(inHandle);
		p = (unsigned char *) *inHandle;
		
		while(i--)
		{	(t[*p++])++;
		}
		
		CreateSymbolTable();
		
		WriteCompressed();
	
	}
	else
	{	outHandle = NewHandle(0);
	}

	UnlockThis();

	return	outHandle;
}

void	CHuffmanHandler::DecodeAll(
	unsigned char	*source,
	unsigned char	*dest)
{
	long			i;
	long			bitData;
	long			*bitPointer;
	long			bitPosition = 0;
	HuffTreeNode	*theNode;
	short			theCode;
	
	i = dataCount;
	while(i--)
	{	//	Read in some bits into bitData.
		bitPointer = (long *)(source + (bitPosition >> 3));

#ifdef INTEL_ARCH
		{	unsigned char	*charPointer = (unsigned char *)bitPointer;
		
			bitData = 	(	((unsigned long)charPointer[0] << 24) |
							((unsigned long)charPointer[1] << 16) |
							((unsigned long)charPointer[2] << 8)  |
							 (unsigned long)charPointer[3]			) << (bitPosition & 7);
		}
#else
		bitData = (*bitPointer) << (bitPosition & 7);
#endif			
		
		//	Use the lookup table to see what these bits decode to:
		theNode = lookupBuf[(bitData >> (32 - HUFFHANDLELOOKUPBITS)) & (HUFFHANDLELOOKUPSIZE-1)];
		
		//	If we didn't reach a leaf node, proceed bit by bit.
		if(theNode->left >= 0)
		{	long		mask = 1L << (31 - HUFFHANDLELOOKUPBITS);
		
			do	//	Decode one bit at a time and walk the tree.
			{
				if(mask & bitData)	theNode = nodes + theNode->right;
					else			theNode = nodes + theNode->left;
				
				mask >>= 1;
			} while(theNode->left >= 0);
		}
		
		//	Move the decoded symbol to theCode
		theCode = theNode->right;
		*dest++ = theCode;
		
		//	Mark n bits as read.
		bitPosition += codeLengths[theCode];
	}
}

Handle	CHuffmanHandler::Uncompress(
	Handle	sourceHandle)
{
	dataCount = GetUncompressedLen(sourceHandle);
	outHandle = NewHandle(dataCount);

	if(outHandle)
	{	HLock(outHandle);
		UncompressTo(sourceHandle, *outHandle);
		HUnlock(outHandle);
	}

	return outHandle;
}

long	CHuffmanHandler::GetUncompressedLen(
	Handle	sourceHandle)
{
	HuffHandleHeader	*inHeader;

	if(GetHandleSize(sourceHandle))
	{	inHeader = (HuffHandleHeader *) *sourceHandle;
		dataCount = MEM_LONG(inHeader->decodedSize);
	}
	else
	{	dataCount = 0;
	}
	
	return dataCount;
}

void	CHuffmanHandler::UncompressTo(
	Handle	sourceHandle,
	Ptr		toPtr)
{
	HuffHandleHeader	*inHeader;
	unsigned long		*freqCounts;
	unsigned short		*freqCountsShort;
	short				i,j,k;
	HuffTreeNode		*lookupMem[HUFFHANDLELOOKUPSIZE];
	short				lengthsArray[NUMSYMBOLS];
	
	if(GetHandleSize(sourceHandle))
	{
		LockThis();
		lookupBuf = lookupMem;
		codeLengths = lengthsArray;
		inHandle = sourceHandle;
	
		inHeader = (HuffHandleHeader *) *sourceHandle;
		dataCount = MEM_LONG(inHeader->decodedSize);
		outPointer = toPtr;
		
		if(toPtr)
		{
			inHeader = (HuffHandleHeader *) *sourceHandle;
			freqCounts =  (unsigned long *)(sizeof(HuffHandleHeader) + *sourceHandle);
			
			k = 0;
	
			freqCountsShort = (unsigned short *)freqCounts;
	
			for(i = 0;i < (NUMSYMBOLS>>5); i++)
			{	long	flags;
				
				flags = MEM_LONG(inHeader->countBitmap[i]);
				
				for(j=0 ; j<32 ; j++)
				{	if(dataCount > 65535)
					{	long	temp = ((flags < 0) ? *freqCounts++ : 0);
						symbCounters[k++] = MEM_LONG(temp);
					}
					else
					{	short	temp = (flags < 0) ? *freqCountsShort++ : 0;
						symbCounters[k++] = MEM_SHORT(temp);
					}
					flags += flags;
				}
			}
	
			if(dataCount <= 65535)
				freqCounts = (unsigned long *) freqCountsShort;
	
			CreateLookupBuffer();
			
			if(singleSymbolData)
			{	Ptr	p = toPtr;
			
				while(dataCount--)
				{	*p++ = theSingleSymbol;
				}
			}
			else
			{	DecodeAll((unsigned char *) freqCounts, (unsigned char *)toPtr);
			}
		}
	
		UnlockThis();
	}
}