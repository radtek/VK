#pragma once
#include "IRunMode.h"
class MacroMode :
	public IRunMode
{
public:
	MacroMode();
	~MacroMode();

	// Odziedziczono za pośrednictwem elementu IRunMode
	virtual void Run() override;
};

