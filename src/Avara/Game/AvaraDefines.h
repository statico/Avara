/*/
    Copyright �1995-1996, Juri Munkki
    All rights reserved.

    File: AvaraDefines.h
    Created: Monday, March 6, 1995, 3:30
    Modified: Thursday, August 22, 1996, 3:18
/*/

#pragma once

#define	kMaxAvaraPlayers		6
#define	kMaxTeamColors			6

enum
{	kStatusTextAndBorderColor = 1,
	kShadowGrayColor,
	kAvaraBackColor,
	kStatusGreenColor,
	kStatusRedColor,
	kFirstPlayerColor,
	kFirstBrightColor = kFirstPlayerColor+kMaxTeamColors,
	kRedGrafColor = kFirstBrightColor+kMaxTeamColors,
	kGreenGrafColor,
	kBlueGrafColor,
	kFirstIndicatorColor,
	kLastIndicatorColor = kFirstIndicatorColor+3
};

#define	kStatusAreaHeight		0	/*29*/
#define	kIndicatorAreaWidth		80
#define	kIndicatorLabelWidth	11
#define	kFirstStatusLine		12
#define	kStatusLineHeight		11
#define	kSecondStatusLine		(kFirstStatusLine+kStatusLineHeight)
#define	kIndicatorBarHeight		9

#define	kEnergyTop				4
#define	kEnergyLine				kFirstStatusLine

#define	kShieldsTop				(kEnergyTop+kIndicatorBarHeight+2)
#define	kShieldsLine			kSecondStatusLine
#define	kBarLabelOffset			9


#define	kScoreTop				4
#define	kScoreLine				kFirstStatusLine

#define	kTimeTop				(kScoreTop+kIndicatorBarHeight+2)
#define	kTimeLine				kSecondStatusLine

#define	kMessageAreaMargin		3

#define	STANDARDMISSILERANGE	FIX(100)
#define	LONGYON					FIX(180)
#define	SHORTYON				FIX(75)

#define	kSliverSizes			3
enum	{	kSmallSliver, kMediumSliver, kLargeSliver };

#define	kAvaraBroadcastDDPSocket	0x67	/*	Just a number off the top of my head. (I was born in 1967) */
#define	kAvaraDDPProtocolOffset		128

#define	kDefaultTraction		FIX3(400)
#define	kDefaultFriction		FIX3(100)
