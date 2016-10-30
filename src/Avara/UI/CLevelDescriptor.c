/*/
    Copyright �1994-1995, Juri Munkki
    All rights reserved.

    File: CLevelDescriptor.c
    Created: Wednesday, November 30, 1994, 8:15
    Modified: Monday, August 21, 1995, 5:07
/*/

#include "CLevelDescriptor.h"
#include "CharWordLongPointer.h"
#include "CTagBase.h"
#include "CAvaraApp.h"
#include "Editions.h"
#include "Script.h"

Handle	FetchEditionPICT(
	FSSpec	*theFile)
{
	OSErr					theErr;
	EditionContainerSpec	container;
	SectionHandle			sectionH = NULL;
	EditionRefNum			refNum = 0;
	Handle					result = NULL;
	
	container.theFile = *theFile;
	container.theFileScript = smRoman;
	container.thePart = kPartsNotUsed;
	container.thePartName[0] = 0;
	container.thePartScript = smRoman;

	theErr = NewSection(&container, theFile, stSubscriber, 1, sumManual, &sectionH);	
	
	if(theErr == noErr)
	{	theErr = OpenEdition(sectionH, &refNum);
		if(theErr == noErr)
		{	Size		formatSize;
		
			theErr = EditionHasFormat(refNum, 'PICT', &formatSize);
			if(theErr == noErr)
			{	result = NewHandle(formatSize);
				if(result)
				{	HLock(result);
					theErr = ReadEdition(refNum, 'PICT', *result, &formatSize);
					HUnlock(result);
				}
			}
			CloseEdition(refNum, theErr == noErr);
		}
		
		theErr = UnRegisterSection(sectionH);
		
		DisposeHandle((Handle)(*sectionH)->alias);
		DisposeHandle((Handle)sectionH);
	}
	return result;
}


CLevelDescriptor *LoadLevelListFromResource(
	OSType	*currentDir)
{
	Handle				theRes;
	CLevelDescriptor	*firstLevel = NULL;
	charWordLongP		uniP;
	long				directoryTag;
	
	theRes = GetResource(LEVELLISTRESTYPE, LEVELLISTRESID);

	if(theRes)
	{	short	levelCount;
		
		MoveHHi(theRes);
		HLock(theRes);
		uniP.c = *theRes;
		*currentDir = *uniP.l++;
		levelCount = *uniP.w++;
		
		firstLevel = new CLevelDescriptor;
		firstLevel->ILevelDescriptor(uniP.c, levelCount);
		
		HUnlock(theRes);
		ReleaseResource(theRes);
	}
	
	return firstLevel;
}

void	CLevelDescriptor::ILevelDescriptor(
	Ptr levelInfo,
	short levelsLeft)
{
	charWordLongP	uniP;
	short			i;
	
	orderNumber = levelsLeft;
	uniP.c = levelInfo;

	tag = *(uniP.l++);
	CopyEvenPascalString(&uniP, name, sizeof(name));
	CopyEvenPascalString(&uniP, intro, sizeof(intro));
	CopyEvenPascalString(&uniP, access, sizeof(access));
	enablesNeeded = *(uniP.w++);
	fromFile = (*(uniP.uw++))>>8;
	
	uniP.l++;	//	Skip reserved data
	
	countEnables = *(uniP.w++);
	if(countEnables > MAXENABLES) countEnables = MAXENABLES;
	
	for(i=0;i<countEnables;i++)
	{	winEnables[i] = *(uniP.l++);
	}

	if(--levelsLeft)
	{	levelInfo = uniP.c;
		nextLevel = new CLevelDescriptor;
		nextLevel->ILevelDescriptor(levelInfo, levelsLeft);
	}
	else
	{	nextLevel = NULL;
	}
}

void	CLevelDescriptor::Dispose()
{
	if(nextLevel)
	{	nextLevel->Dispose();
	}
	
	inherited::Dispose();
}

void	CLevelDescriptor::UpdateLevelInfoTags(
	CTagBase			*theTags,
	CLevelDescriptor	*levelList)
{
	LevelScoreRecord	levelRecord;
	long				len = sizeof(LevelScoreRecord);

	if(theTags->ReadEntry(tag, &len, &levelRecord) >= 0)
	{	
		levelRecord.orderNumber = orderNumber;
		levelRecord.isEnabled = enablesReceived >= enablesNeeded;
		
		if(levelRecord.showResults || levelRecord.isEnabled)
		{	theTags->WriteEntry(tag, sizeof(LevelScoreRecord), &levelRecord);
		}
		else
		{	theTags->ReleaseData(tag);
		}
	}
	else
	{	BlockMoveData(name, levelRecord.name, name[0]+1);
		levelRecord.orderNumber = orderNumber;
		levelRecord.tag = tag;
		levelRecord.hasWon = false;
		levelRecord.showResults = false;
		levelRecord.first.by = -1;
		levelRecord.first.score = 0;
		levelRecord.first.time = -1;
		levelRecord.first.when = 0;
		levelRecord.fast = levelRecord.first;
		levelRecord.high = levelRecord.first;
		levelRecord.isEnabled = enablesReceived >= enablesNeeded;

		if(levelRecord.isEnabled)
		{	theTags->WriteEntry(tag, sizeof(LevelScoreRecord), &levelRecord);
		}
	}

	
	if(nextLevel)
		nextLevel->UpdateLevelInfoTags(theTags, levelList);
}

void	CLevelDescriptor::PrepareForLevelTagUpdate()
{
	enablesReceived = 0;
	if(nextLevel)
		nextLevel->PrepareForLevelTagUpdate();
}

void	CLevelDescriptor::IncreaseEnableCounts(
	short		count,
	OSType		*enableList)
{
	if(nextLevel)
	{	nextLevel->IncreaseEnableCounts(count, enableList);
	}
	
	while(count--)
	{	if(*enableList++ == tag)
		{	enablesReceived++;
		}
	}
}

void	CLevelDescriptor::UpdateEnableCounts(
	CTagBase			*theTags,
	CLevelDescriptor	*levelList)
{
	LevelScoreRecord	levelRecord;
	long				len = sizeof(LevelScoreRecord);

	if(theTags->ReadEntry(tag, &len, &levelRecord) >= 0)
	{	
		if(levelRecord.hasWon)
		{	levelList->IncreaseEnableCounts(countEnables, winEnables);
		}
	}
	
	if(nextLevel)
	{	nextLevel->UpdateEnableCounts(theTags, levelList);
	}
	else
	{	//	Last leveldescriptor will clear all the enable flags in the tagbase.
		LevelScoreRecord	*rec;
		long				key = 0;
		
		while(rec = theTags->GetNextPointer(&key))
		{	rec->isEnabled = false;
		}
	}

}

void	CLevelDescriptor::FindDescriptor(
	OSType		theTag,
	StringPtr	description)
{
	if(theTag == tag)	BlockMoveData(intro, description, intro[0]+1);
	else
		if(nextLevel)	nextLevel->FindDescriptor(theTag, description);
		else			description[0] = 0;
}

void	CLevelDescriptor::FindLevelInfo(
	OSType				theTag,
	LevelScoreRecord 	*newRecord)
{
	if(theTag == tag || nextLevel == NULL)
	{	BlockMoveData(name, &newRecord->name, name[0]+1);
		newRecord->tag = theTag;
		newRecord->hasWon = false;
		newRecord->isEnabled = true;
		newRecord->showResults = false;
		newRecord->first.score = 0;
		newRecord->first.time = -1;
		newRecord->first.when = 0;
		newRecord->first.by = 0;
		newRecord->fast = newRecord->first;
		newRecord->high = newRecord->first;
		newRecord->orderNumber = orderNumber;
	}
	else
		if(nextLevel)	nextLevel->FindLevelInfo(theTag, newRecord);
}

Handle	CLevelDescriptor::GetLevelData(
	OSType		theTag,
	StringPtr	levelName)
{
	Handle	result = NULL;

	if(theTag == tag)
	{	BlockMoveData(name, levelName, name[0]+1);

		if(fromFile)
		{	FSSpec	theFile;
			long	fileLen;
			OSErr	theErr;
			short	ref;
			FInfo	fndrInfo;
		
			theFile = ((CAvaraApp *)gApplication)->directorySpec;
			BlockMoveData(access, theFile.name, access[0]+1);
			
			fndrInfo.fdType = 0;
			theErr = FSpGetFInfo(&theFile, &fndrInfo);
			switch(fndrInfo.fdType)
			{	case 'PICT':
					theErr = FSpOpenDF(&theFile, fsRdPerm, &ref);
					if(theErr == noErr)
					{	theErr = GetEOF(ref, &fileLen);
						if(theErr == noErr)
						{	fileLen -= 512;
							theErr = SetFPos(ref, fsFromStart, 512);
							if(theErr == noErr)
							{	result = NewHandle(fileLen);
								if(result)
								{	HLock(result);
									theErr = FSRead(ref, &fileLen, *result);
									if(theErr != noErr)
									{	DisposeHandle(result);
										result = NULL;
									}
									else
									{	HUnlock(result);
									}
								}
							}
						}
						FSClose(ref);
					}
					break;
				case kPICTEditionFileType:
					result = FetchEditionPICT(&theFile);
					break;
			}
		}
		else
		{	result = GetNamedResource('PICT', access);
			if(result)
			{	DetachResource(result);
			}
		}
	}
	else
	{	if(nextLevel)
		{	result = nextLevel->GetLevelData(theTag, levelName);
		}
	}
	
	return result;
}