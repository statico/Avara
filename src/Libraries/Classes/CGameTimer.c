/*/
    Copyright �1994-1996, Juri Munkki
    All rights reserved.

    File: CGameTimer.c
    Created: Sunday, November 20, 1994, 0:21
    Modified: Tuesday, June 18, 1996, 22:15
/*/

#include "CGameTimer.h"

#pragma options(!profile)

static
pascal	void GameTimerTask()
{
	GameTimerRecord		*vars;
	long				thisStepTime;
	long				stepDelta;
	
	asm	{	move.l	a1,vars	};

	thisStepTime = vars->stepTime;
#define noVARIBLESTEPTIMES
#ifdef VARIABLESTEPTIMES
	stepDelta = vars->currentStep - vars->stepCount;

	if(stepDelta > 5)
	{	if(stepDelta > 10)
		{	if(stepDelta > 20)
			{	vars->stepCount = vars->currentStep;
			}

			thisStepTime += thisStepTime;
		}
		else
		{	thisStepTime += thisStepTime >> 1;
		}
	}
#endif
	
	vars->stepCount++;
	PrimeTime((QElemPtr)&vars->task, thisStepTime);
}

void	CGameTimer::ResetTiming()
{
	RmvTime((QElemPtr)&timer.task);
	timer.currentStep = timer.stepCount;
	InsXTime((QElemPtr)&timer.task);
	PrimeTime((QElemPtr)&timer.task, timer.stepTime);
}

void	CGameTimer::SetFrameTiming(
	long	frameTime)
{
	RmvTime((QElemPtr)&timer.task);
	timer.stepTime = frameTime;
	InsXTime((QElemPtr)&timer.task);
	PrimeTime((QElemPtr)&timer.task, timer.stepTime);
}

static
void	TimerExit(
	long theData)
{
	gExitHandler->RemoveExit(&((CGameTimer *) theData)->theExit);

	((CGameTimer *)	theData)->Dispose();
}
void	CGameTimer::IGameTimer()
{
	theExit.theData = (long)this;
	theExit.exitFunc = TimerExit;
	StartExitHandler();
	gExitHandler->AddExit(&theExit);
	
	timer.task.tmAddr = (void *) GameTimerTask;
	timer.task.tmReserved = 0;
	timer.task.tmWakeUp = 0;
	timer.stepCount = 0;
	timer.currentStep = 0;
	timer.stepTime = 33;	//	33 milliseconds means 30 frames per second.

	InsXTime((QElemPtr)&(timer.task));
	PrimeTime((QElemPtr)&timer.task, timer.stepTime);
}

void	CGameTimer::Dispose()
{
	gExitHandler->RemoveExit(&theExit);

	RmvTime((QElemPtr)&(timer.task));

	inherited::Dispose();
}

long	CGameTimer::GetStepCount()
{
	return timer.stepCount;
}

void	CGameTimer::StopTimer()
{
	RmvTime((QElemPtr)&timer.task);
}

void	CGameTimer::ResumeTimer()
{
	long	timeRemaining = timer.task.tmCount;

	InsXTime((QElemPtr)&timer.task);
	timer.task.tmReserved = 0;
	timer.task.tmWakeUp = 0;
	PrimeTime((QElemPtr)&timer.task, timeRemaining);
}