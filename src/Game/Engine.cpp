#include "stdafx.h"
#if defined(_WIN32)
#include "Engine.h"
#include "Core.h"
//=============================================================================
#if defined(_WIN32)
extern "C"
{
	__declspec(dllexport) unsigned long NvOptimusEnablement = 1;
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif
//=============================================================================
namespace
{
	uint16_t windowWidth;
	uint16_t windowHeight;
	bool     windowIsResize{ false };

	bool IsRunningApp{ false };

	constexpr const wchar_t* WindowClassName{ L"EngineWindowClass" };
	HWND hwnd{ nullptr };
	MSG msg{};

	std::chrono::steady_clock::time_point lastFrameTime{};
	float deltaTime{ 0.0f };
}
//=============================================================================
bool InitializeRender(HWND hwnd, uint16_t wndWidth, uint16_t wndHeight);
void ShutdownRender();
void RenderResize(uint16_t wndWidth, uint16_t wndHeight);
void RenderSwap();
//=============================================================================
void ExitEngineApp()
{
	IsRunningApp = false;
}
//=============================================================================
LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept
{
	switch (uMsg)
	{
	case WM_CLOSE:
		ExitEngineApp();
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_SIZE:
		{
			int width = LOWORD(lParam);
			int height = HIWORD(lParam);
			if (width > 0 && height > 0)
			{
				windowWidth = static_cast<uint16_t>(width);
				windowHeight = static_cast<uint16_t>(height);
				windowIsResize = true;
			}
		}
		break;
	case WM_EXITSIZEMOVE:
		break;
	default:
		break;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}
//=============================================================================
bool engine::Initialize(uint16_t wndWidth, uint16_t wndHeight, const wchar_t* windowTitle)
{
	// Validate parameters
	{
		if (windowTitle == nullptr)
		{
			Fatal("Window title cannot be null!");
			return false;
		}
		if (wndWidth == 0)
		{
			Fatal("Window width cannot be zero!");
			return false;
		}
		if (wndHeight == 0)
		{
			Fatal("Window height cannot be zero!");
			return false;
		}
	}

	WNDCLASSEX wndclassex{ .cbSize = sizeof(WNDCLASSEX) };
	wndclassex.style         = CS_HREDRAW | CS_VREDRAW;
	wndclassex.lpfnWndProc   = WindowProc;
	wndclassex.hCursor       = LoadCursor(nullptr, IDC_ARROW);
	wndclassex.lpszClassName = WindowClassName;
	if (!RegisterClassEx(&wndclassex))
	{
		Fatal("Failed to register window class!");
		return false;
	}

	RECT initialRect = { 0, 0, wndWidth, wndHeight };
	DWORD winStyle = WS_OVERLAPPEDWINDOW;
	DWORD winStyleEx = WS_EX_OVERLAPPEDWINDOW;
	AdjustWindowRectEx(&initialRect, winStyle, false, winStyleEx);
	LONG initialWidth = initialRect.right - initialRect.left;
	LONG initialHeight = initialRect.bottom - initialRect.top;

	hwnd = CreateWindowEx(winStyleEx, WindowClassName, windowTitle, winStyle,
		CW_USEDEFAULT, CW_USEDEFAULT,
		initialWidth, initialHeight,
		nullptr, nullptr, nullptr, nullptr);
	if (!hwnd)
	{
		Fatal("Failed to create window!");
		return false;
	}
	ShowWindow(hwnd, SW_SHOW);

	windowWidth = wndWidth;
	windowHeight = wndHeight;
	windowIsResize = false;

	lastFrameTime = std::chrono::steady_clock::now();
	deltaTime = 0.0f;

	if (!InitializeRender(hwnd, windowWidth, windowHeight))
	{
		Fatal("Failed to initialize rendering!");
		return false;
	}

	IsRunningApp = true;
	return true;
}
//=============================================================================
void engine::Shutdown()
{
	IsRunningApp = false;
	ShutdownRender();
	if (hwnd) DestroyWindow(hwnd);
	hwnd = nullptr;
}
//=============================================================================
bool engine::IsRunning()
{
	return IsRunningApp;
}
//=============================================================================
bool engine::ProcessEvents()
{
	if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
	{
		if (msg.message == WM_QUIT)
		{
			return false;
		}
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return IsRunningApp;
}
//=============================================================================
void engine::BeginFrame()
{
	auto currentTime = std::chrono::steady_clock::now();
	deltaTime = std::chrono::duration<float>(currentTime - lastFrameTime).count();
	lastFrameTime = currentTime;

	if (windowIsResize)
	{
		RenderResize(windowWidth, windowHeight);
		windowIsResize = false;
	}
}
//=============================================================================
void engine::EndFrame()
{
	RenderSwap();
	wchar_t title[100];
	float currentFPS = 1.0f / std::max(deltaTime, 0.0001f);
	swprintf_s(title, L"Engine - FPS: %.2f, DeltaTime: %.4f", currentFPS, deltaTime);
	SetWindowText(hwnd, title);
}
//=============================================================================
float engine::GetDeltaTime()
{
	return deltaTime;
}
//=============================================================================
#endif // _WIN32