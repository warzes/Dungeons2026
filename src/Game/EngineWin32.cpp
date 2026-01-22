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
	uint16_t frameBufferWidth;
	uint16_t frameBufferHeight;

	bool IsRunningApp{ false };

	constexpr const wchar_t* WindowClassName{ L"RetroEngineWindowClass" };
	HINSTANCE hInstance{ nullptr };
	HWND hwnd{ nullptr };
	MSG msg{};

	std::chrono::steady_clock::time_point lastFrameTime{};
	float deltaTime{ 0.0f };
}
//=============================================================================
void setWindowSize(uint16_t width, uint16_t height)
{
	windowWidth = width;
	windowHeight = height;

	const float aspect = static_cast<float>(width) / static_cast<float>(height);
	frameBufferWidth = static_cast<uint16_t>(static_cast<float>(frameBufferHeight) * aspect);
}
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
				setWindowSize(static_cast<uint16_t>(width), static_cast<uint16_t>(height));
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
bool engine::Initialize(uint16_t wndWidth, uint16_t wndHeight, const wchar_t* windowTitle, uint16_t fbHeight)
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
		if (fbHeight == 0)
		{
			Fatal("Frame buffer height cannot be zero!");
			return false;
		}
		if (fbHeight > wndHeight)
		{
			Warning("Frame buffer height is greater than window height. Clamping frame buffer height to window height.");
			fbHeight = wndHeight;
		}
	}

	frameBufferHeight = fbHeight;
	setWindowSize(wndWidth, wndHeight);

	hInstance = GetModuleHandle(nullptr);

	WNDCLASSEX wndclassex{ .cbSize = sizeof(WNDCLASSEX) };
	wndclassex.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wndclassex.lpfnWndProc   = WindowProc;
	wndclassex.hInstance     = hInstance;
	wndclassex.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
	wndclassex.hCursor       = LoadCursor(nullptr, IDC_ARROW);
	wndclassex.lpszClassName = WindowClassName;
	if (!RegisterClassEx(&wndclassex))
	{
		Fatal("Failed to register window class!");
		return false;
	}

	RECT rect = { 0, 0, windowWidth, windowHeight };
	DWORD winStyleEx = WS_EX_OVERLAPPEDWINDOW;
	DWORD winStyle = WS_OVERLAPPEDWINDOW;
	AdjustWindowRectEx(&rect, winStyle, false, winStyleEx);

	hwnd = CreateWindowEx(winStyleEx, WindowClassName, windowTitle, winStyle,
		CW_USEDEFAULT, CW_USEDEFAULT,
		rect.right - rect.left, rect.bottom - rect.top,
		nullptr, nullptr, hInstance, nullptr);
	if (!hwnd)
	{
		Fatal("Failed to create window!");
		return false;
	}
	ShowWindow(hwnd, SW_SHOW);
	UpdateWindow(hwnd);

	GetClientRect(hwnd, &rect);
	auto clientWidth = (rect.right - rect.left);
	auto clientHeight = (rect.bottom - rect.top);
	if (clientWidth < 0 || clientHeight < 0)
	{
		Fatal("Invalid window rectangle dimensions");
		return false;
	}

	windowWidth = static_cast<uint16_t>(clientWidth);
	windowHeight = static_cast<uint16_t>(clientHeight);

	lastFrameTime = std::chrono::steady_clock::now();
	deltaTime = 0.0f;

	IsRunningApp = true;
	return true;
}
//=============================================================================
void engine::Shutdown()
{
	IsRunningApp = false;

	if (hwnd) DestroyWindow(hwnd);
	if (hInstance) UnregisterClass(WindowClassName, hInstance);
	hwnd = nullptr;
	hInstance = nullptr;
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
}
//=============================================================================
void engine::EndFrame()
{
	wchar_t title[100];
	float currentFPS = 1.0f / deltaTime;
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