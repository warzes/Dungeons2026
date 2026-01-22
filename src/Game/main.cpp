#include "stdafx.h"
#include "GameApp.h"
//=============================================================================
#if defined(_MSC_VER)
#	pragma comment( lib, "3rdparty.lib" )
#	pragma comment( lib, "d3d11.lib" )
#	pragma comment( lib, "d3dcompiler.lib" )
#endif
//=============================================================================
int main(
	[[maybe_unused]] int   argc,
	[[maybe_unused]] char* argv[])
{
	GameApp();
}
//=============================================================================