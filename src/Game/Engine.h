#pragma once

namespace engine
{
	bool Initialize(uint16_t windowWidth, uint16_t windowHeight, const wchar_t* windowTitle, uint16_t frameBufferHeight);
	void Shutdown();

	bool IsRunning();

	bool ProcessEvents();
	void BeginFrame();
	void EndFrame();

	float GetDeltaTime();
} // namespace engine