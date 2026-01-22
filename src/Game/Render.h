#pragma once

enum class PixelFormat : uint8_t
{
	R32Float,
	RG32Float,
	RGB32Float,
	RGBA32Float,
	R8UNorm,
	RG8UNorm,
	RGBA8UNorm
};

enum class VertexFormat : uint8_t
{
	Float1,
	Float2,
	Float3,
	Float4,
	UChar1,
	UChar2,
	UChar4,
	UChar1Normalized,
	UChar2Normalized,
	UChar4Normalized,
};

class RenderTarget
{
};

namespace render
{
	void SetRenderTarget(const std::vector<const RenderTarget*>& value);
	void SetRenderTarget(const RenderTarget& value);
	void SetRenderTarget(std::nullopt_t value);
	void SetRenderTarget(const RenderTarget** render_target, size_t count);
} // namespace render