#include "stdafx.h"
#include "GameApp.h"
#include "Core.h"
#include "EngineMath.h"
#include "Engine.h"
//=============================================================================
namespace
{

}
//=============================================================================
void GameApp()
{
	if (engine::Initialize(1600, 900, L"Game", 120))
	{
		while (engine::IsRunning())
		{
			if (!engine::ProcessEvents())
			{
				break;
			}
			engine::BeginFrame();
			
			engine::EndFrame();
		}
	}
	engine::Shutdown();
}
//=============================================================================