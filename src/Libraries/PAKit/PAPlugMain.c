/*/
    Copyright �1995, Juri Munkki
    All rights reserved.

    File: PAPlugMain.c
    Created: Tuesday, May 2, 1995, 12:14
    Modified: Sunday, November 19, 1995, 11:42
/*/

#define	POWERPLUG_CLASS
#include "MixedMode.h"
#include "PAKit.h"
#include "BSPPlug.h"

PolyWorld	*thePolyWorld;

long	__procinfo = 
		 kThinkCStackBased
		| RESULT_SIZE(SIZE_CODE(sizeof(UniversalProcPtr)))
		| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(short)));

UniversalProcPtr	main(short a);

UniversalProcPtr	main(short a)
{
	UniversalProcPtr	theProc;

	switch(a)
	{	case kPowerScanConvert:
			{	static	RoutineDescriptor	powerScanConvert = BUILD_ROUTINE_DESCRIPTOR(
							kThinkCStackBased
							| RESULT_SIZE(SIZE_CODE(0))
							| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(PolyWorld *))) 
							| STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(short *))) 
							| STACK_ROUTINE_PARAMETER(3, SIZE_CODE(sizeof(PolyEdgePtr *))) 
							| STACK_ROUTINE_PARAMETER(4, SIZE_CODE(sizeof(PolyEdgePtr *))) 
							| STACK_ROUTINE_PARAMETER(5, SIZE_CODE(sizeof(Rect *))) 
							| STACK_ROUTINE_PARAMETER(6, SIZE_CODE(sizeof(short))),
							(ProcPtr)ScanConvertPolysC);
				theProc = &powerScanConvert;
			}
			break;
		case kPowerPADiff:
			{	static	RoutineDescriptor	powerPADiff = BUILD_ROUTINE_DESCRIPTOR(
							kThinkCStackBased
							| RESULT_SIZE(SIZE_CODE(0))
							| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(PolyWorld *))) 
							| STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(short *))) 
							| STACK_ROUTINE_PARAMETER(3, SIZE_CODE(sizeof(short *))) 
							| STACK_ROUTINE_PARAMETER(4, SIZE_CODE(sizeof(short *))) 
							| STACK_ROUTINE_PARAMETER(5, SIZE_CODE(sizeof(short))), 
							(ProcPtr)DiffPolyRegionsC);
				theProc = &powerPADiff;
			}
			break;
		case kPower8BitDriver:
			{	static	RoutineDescriptor	power8BitDriver = BUILD_ROUTINE_DESCRIPTOR(
							kThinkCStackBased
							| RESULT_SIZE(SIZE_CODE(0))
							| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(short *))) 
							| STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(void *))) 
							| STACK_ROUTINE_PARAMETER(3, SIZE_CODE(sizeof(short))) 
							| STACK_ROUTINE_PARAMETER(4, SIZE_CODE(sizeof(void *))) 
							| STACK_ROUTINE_PARAMETER(5, SIZE_CODE(sizeof(short))) 
							| STACK_ROUTINE_PARAMETER(6, SIZE_CODE(sizeof(short))), 
							(ProcPtr)BytePixelDriverC);
				theProc = &power8BitDriver;
			}
			break;
		case kPower16BitDriver:
			{	static	RoutineDescriptor	power16BitDriver = BUILD_ROUTINE_DESCRIPTOR(
							kThinkCStackBased
							| RESULT_SIZE(SIZE_CODE(0))
							| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(short *))) 
							| STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(void *))) 
							| STACK_ROUTINE_PARAMETER(3, SIZE_CODE(sizeof(short))) 
							| STACK_ROUTINE_PARAMETER(4, SIZE_CODE(sizeof(void *))) 
							| STACK_ROUTINE_PARAMETER(5, SIZE_CODE(sizeof(short))) 
							| STACK_ROUTINE_PARAMETER(6, SIZE_CODE(sizeof(short))), 
							(ProcPtr)True16PixelDriverC);
				theProc = &power16BitDriver;
			}
			break;
		case kPower24BitDriver:
			{	static	RoutineDescriptor	power24BitDriver = BUILD_ROUTINE_DESCRIPTOR(
							kThinkCStackBased
							| RESULT_SIZE(SIZE_CODE(0))
							| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(short *))) 
							| STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(void *))) 
							| STACK_ROUTINE_PARAMETER(3, SIZE_CODE(sizeof(short))) 
							| STACK_ROUTINE_PARAMETER(4, SIZE_CODE(sizeof(void *))) 
							| STACK_ROUTINE_PARAMETER(5, SIZE_CODE(sizeof(short))) 
							| STACK_ROUTINE_PARAMETER(6, SIZE_CODE(sizeof(short))), 
							(ProcPtr)True24PixelDriverC);
				theProc = &power24BitDriver;
			}
			break;
		case kBSPDrawPolygons:
			{	static	RoutineDescriptor	bspDrawPolygons = BUILD_ROUTINE_DESCRIPTOR(
							kThinkCStackBased
							| RESULT_SIZE(SIZE_CODE(0))
							| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(DrawPolygonsParam *))),
							(ProcPtr)DrawPolygonsPlugC);
				theProc = &bspDrawPolygons;
			}
			break;
		case kBSPInitPlug:
			{	static	RoutineDescriptor	bspInitPlug = BUILD_ROUTINE_DESCRIPTOR(
							kThinkCStackBased
							| RESULT_SIZE(SIZE_CODE(0))
							| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(BSPGlobalsInfoRecord *))),
							(ProcPtr)InitBSPPlugC);
				theProc = &bspInitPlug;
			}
			break;
		case kBSPPreProcess:
			{	static	RoutineDescriptor	bspPreProcessPlug = BUILD_ROUTINE_DESCRIPTOR(
							kThinkCStackBased
							| RESULT_SIZE(SIZE_CODE(0))
							| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(Handle *))),
							(ProcPtr)BSPPreProcessC);
				theProc = &bspPreProcessPlug;
			}
			break;

		case kBSPPreProcessList:
			{	static	RoutineDescriptor	bspPreProcessList = BUILD_ROUTINE_DESCRIPTOR(
							kThinkCStackBased
							| RESULT_SIZE(SIZE_CODE(0))
							| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(PreProcessListParam *))),
							(ProcPtr)PreProcessListPlugC);
				theProc = &bspPreProcessList;
			}
			break;

		case kBSPDrawPolygonsList:
			{	static	RoutineDescriptor	bspDrawPolygonsList = BUILD_ROUTINE_DESCRIPTOR(
							kThinkCStackBased
							| RESULT_SIZE(SIZE_CODE(0))
							| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(DrawPolygonsParam *))),
							(ProcPtr)DrawPolygonsListPlugC);
				theProc = &bspDrawPolygonsList;
			}
			break;


		default:
			theProc = NULL;
			break;
	}
	
	return theProc;
}
