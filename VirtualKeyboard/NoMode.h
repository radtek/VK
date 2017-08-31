#pragma once
#include "IRunMode.h"
#include <stdexcept>
class NoMode :
	public IRunMode
{
public:
	NoMode();
	~NoMode();

	// Odziedziczono za pośrednictwem elementu IRunMode
	virtual void Run() override { throw std::runtime_error{ "No Running mode specified" };}
	virtual bool RequiresConnection() override { return false; };
};

