/*/
    Copyright �1996, Juri Munkki
    All rights reserved.

    File: CYonBox.h
    Created: Thursday, August 22, 1996, 3:00
    Modified: Thursday, August 22, 1996, 3:01
/*/

#pragma once
#include "CAbstractYon.h"

class	CYonBox : public CAbstractYon
{
public:
	virtual	void			BeginScript();
	virtual	CAbstractActor *EndScript();
};