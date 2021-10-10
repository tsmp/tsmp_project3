#pragma once
#include "ConsoleRender.h"

class dxConsoleRender : public IConsoleRender
{
public:
	dxConsoleRender();

	virtual void Copy(IConsoleRender &_in);
	virtual void OnRender(bool bGame);
};
