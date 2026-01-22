#pragma once

#include "3rdparty/3rdpartyConfig.h"
#include "GameConfig.h"

#define _USE_MATH_DEFINES

#if defined(_MSC_VER)
#	pragma warning(disable : 4514)
#	pragma warning(disable : 5045)
#	pragma warning(push, 3)
#	pragma warning(disable : 4865)
#	pragma warning(disable : 5039)
#endif

#include <cassert>
#include <cmath>

#include <random>
#include <chrono>
//#include <algorithm>
//#include <array>
//#include <list>
//#include <vector>
#include <unordered_map>
#include <string>

#if defined(_WIN32)
#	define NOGDICAPMASKS
#	define NOVIRTUALKEYCODES
//#	define NOWINMESSAGES
//#	define NOWINSTYLES
#	define NOSYSMETRICS
#	define NOMENUS
#	define NOICONS
#	define NOKEYSTATES
#	define NOSYSCOMMANDS
#	define NORASTEROPS
//#	define NOSHOWWINDOW
#	define OEMRESOURCE
#	define NOATOM
#	define NOCLIPBOARD
//#	define NOCOLOR
#	define NOCTLMGR
#	define NODRAWTEXT
//#	define NOGDI
#	define NOKERNEL
//#	define NOUSER 
#	define NONLS
#	define NOMB
#	define NOMEMMGR
#	define NOMETAFILE
#	define NOMINMAX
//#	define NOMSG
#	define NOOPENFILE
#	define NOSCROLL
#	define NOSERVICE
#	define NOSOUND
#	define NOTEXTMETRIC
#	define NOWH
#	define NOWINOFFSETS
#	define NOCOMM
#	define NOKANJI
#	define NOHELP
#	define NOPROFILER
#	define NODEFERWINDOWPOS
#	define NOMCX
#	define WIN32_LEAN_AND_MEAN

#	include <Windows.h>
#	include <wrl.h>
#	include <d3d11_4.h>
#	include <d3dcompiler.h>
#	include <dxgi1_6.h>
#endif

#if defined(_MSC_VER)
#	pragma warning(pop)
#endif