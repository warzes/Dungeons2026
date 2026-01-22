#include "stdafx.h"
#include "Core.h"
//=============================================================================
void Print(const std::string& msg)
{
	puts(msg.c_str());
}
//=============================================================================
void Warning(const std::string& msg)
{
	Print("\033[33m[WARNING]:\033[0m " + msg);
}
//=============================================================================
void Error(const std::string& msg)
{
	Print("\033[31m[ERROR]:\033[0m " + msg);
}
//=============================================================================
void Fatal(const std::string& msg)
{
	void ExitEngineApp();

	ExitEngineApp();
	Print("\033[35m[FATAL]:\033[0m " + msg);
}
//=============================================================================