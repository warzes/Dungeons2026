#include "stdafx.h"
#if defined(_WIN32)
#include "Render.h"
#include "Core.h"
//=============================================================================
using Microsoft::WRL::ComPtr;
//=============================================================================
class ShaderD3D11;
class TextureD3D11;
class RenderTargetD3D11;
//=============================================================================
namespace
{
	uint16_t                        windowWidth{ 0 };
	uint16_t                        windowHeight{ 0 };
	ComPtr<ID3D11Device>            d3d11Device{ nullptr };
	ComPtr<ID3D11DeviceContext>     d3d11DeviceContext{ nullptr };
	ComPtr<IDXGISwapChain>          d3d11SwapChain{ nullptr };
	
	TextureD3D11*                   backbufferTexture{ nullptr };
	RenderTargetD3D11*              mainRenderTarget{ nullptr };
	std::vector<RenderTargetD3D11*> renderTargets;


	
	ID3D11RenderTargetView*     d3d11FrameBufferView{ nullptr };

	ID3DBlob* vsBlob;
	ID3D11VertexShader* vertexShader;
	ID3D11PixelShader* pixelShader;
	ID3D11InputLayout* inputLayout;

	ID3D11Buffer* vertexBuffer;
	UINT numVerts;
	UINT stride;
	UINT offset;

	ID3D11SamplerState* samplerState;
	ID3D11Texture2D* texture;
	ID3D11ShaderResourceView* textureView;
}
//=============================================================================
inline uint32_t GetMipWidth(uint32_t base_width, uint32_t mip_level)
{
	return std::max<uint32_t>(1, static_cast<uint32_t>(std::floor<uint32_t>(base_width >> mip_level)));
}
//=============================================================================
inline uint32_t GetMipHeight(uint32_t base_height, uint32_t mip_level)
{
	return std::max<uint32_t>(1, static_cast<uint32_t>(std::floor<uint32_t>(base_height >> mip_level)));
}
//=============================================================================
inline DXGI_FORMAT EnumToD3D11(PixelFormat format)
{
	switch (format)
	{
	case PixelFormat::R32Float:    return DXGI_FORMAT_R32_FLOAT;
	case PixelFormat::RG32Float:   return DXGI_FORMAT_R32G32_FLOAT;
	case PixelFormat::RGB32Float:  return DXGI_FORMAT_R32G32B32_FLOAT;
	case PixelFormat::RGBA32Float: return DXGI_FORMAT_R32G32B32A32_FLOAT;
	case PixelFormat::R8UNorm:     return DXGI_FORMAT_R8_UNORM;
	case PixelFormat::RG8UNorm:    return DXGI_FORMAT_R8G8_UNORM;
	case PixelFormat::RGBA8UNorm:  return DXGI_FORMAT_R8G8B8A8_UNORM;
	default: std::unreachable();
	}
}
//=============================================================================
inline uint32_t GetFormatChannelsCount(PixelFormat format)
{
	switch (format)
	{
	case PixelFormat::R32Float:    return 1;
	case PixelFormat::RG32Float:   return 2;
	case PixelFormat::RGB32Float:  return 3;
	case PixelFormat::RGBA32Float: return 4;
	case PixelFormat::R8UNorm:     return 1;
	case PixelFormat::RG8UNorm:    return 2;
	case PixelFormat::RGBA8UNorm:  return 4;
	default: std::unreachable();
	}
}
//=============================================================================
inline uint32_t GetFormatChannelSize(PixelFormat format)
{
	switch (format)
	{
	case PixelFormat::R32Float:    return 4;
	case PixelFormat::RG32Float:   return 4;
	case PixelFormat::RGB32Float:  return 4;
	case PixelFormat::RGBA32Float: return 4;
	case PixelFormat::R8UNorm:     return 1;
	case PixelFormat::RG8UNorm:    return 1;
	case PixelFormat::RGBA8UNorm:  return 1;
	default: std::unreachable();
	}
}
//=============================================================================
class ShaderD3D11 final
{

};
//=============================================================================
class TextureD3D11 final
{
public:
	TextureD3D11(uint32_t width, uint32_t height, PixelFormat format, uint32_t mip_count)
		: m_width(width)
		, m_height(height)
		, m_format(format)
		, m_mipCount(mip_count)
	{
		auto tex_desc = CD3D11_TEXTURE2D_DESC(EnumToD3D11(format), width, height);
		tex_desc.MipLevels = mip_count;
		tex_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
		tex_desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
		d3d11Device->CreateTexture2D(&tex_desc, NULL, m_texture2D.GetAddressOf());

		auto srv_desc = CD3D11_SHADER_RESOURCE_VIEW_DESC(m_texture2D.Get(), D3D11_SRV_DIMENSION_TEXTURE2D);
		d3d11Device->CreateShaderResourceView(m_texture2D.Get(), &srv_desc, m_shaderResourceView.GetAddressOf());
	}

	TextureD3D11(uint32_t width, uint32_t height, PixelFormat format, ComPtr<ID3D11Texture2D> texture) :
		m_width(width),
		m_height(height),
		m_format(format),
		m_texture2D(texture)
	{
	}

	void Write(uint32_t width, uint32_t height, const void* memory, uint32_t mip_level, uint32_t offset_x, uint32_t offset_y)
	{
		auto channels = GetFormatChannelsCount(m_format);
		auto channel_size = GetFormatChannelSize(m_format);
		auto mem_pitch = width * channels * channel_size;
		auto mem_slice_pitch = width * height * channels * channel_size;
		auto dst_box = CD3D11_BOX(offset_x, offset_y, 0, offset_x + width, offset_y + height, 1);
		d3d11DeviceContext->UpdateSubresource(m_texture2D.Get(), mip_level, &dst_box, memory, mem_pitch,
			mem_slice_pitch);
	}

	std::vector<uint8_t> Read(uint32_t mip_level) const
	{
		auto mip_width = GetMipWidth(m_width, mip_level);
		auto mip_height = GetMipHeight(m_height, mip_level);

		CD3D11_TEXTURE2D_DESC staging_desc(EnumToD3D11(m_format), mip_width, mip_height, 1, 1, 0, D3D11_USAGE_STAGING,
			D3D11_CPU_ACCESS_READ);

		ComPtr<ID3D11Texture2D> staging_texture;
		d3d11Device->CreateTexture2D(&staging_desc, nullptr, &staging_texture);

		d3d11DeviceContext->CopySubresourceRegion(staging_texture.Get(), 0, 0, 0, 0, m_texture2D.Get(), mip_level, nullptr);

		D3D11_MAPPED_SUBRESOURCE mapped_resource;
		d3d11DeviceContext->Map(staging_texture.Get(), 0, D3D11_MAP_READ, 0, &mapped_resource);

		auto channels_count = GetFormatChannelsCount(m_format);
		auto channel_size = GetFormatChannelSize(m_format);
		size_t row_size = mip_width * channels_count * channel_size;
		std::vector<uint8_t> result(mip_height * row_size);

		uint32_t row_pitch = mapped_resource.RowPitch;

		for (uint32_t y = 0; y < mip_height; ++y)
		{
			auto src_row = static_cast<const uint8_t*>(mapped_resource.pData) + y * row_pitch;
			auto dst_row = result.data() + y * row_size;
			memcpy(dst_row, src_row, row_size);
		}

		d3d11DeviceContext->Unmap(staging_texture.Get(), 0);

		return result;
	}

	void GenerateMips()
	{
		d3d11DeviceContext->GenerateMips(m_shaderResourceView.Get());
	}

	const auto& GetD3D11Texture2D() const { return m_texture2D; }
	const auto& GetD3D11ShaderResourceView() const { return m_shaderResourceView; }
	auto GetWidth() const { return m_width; }
	auto GetHeight() const { return m_height; }
	auto GetFormat() const { return m_format; }
private:
	ComPtr<ID3D11ShaderResourceView> m_shaderResourceView;
	ComPtr<ID3D11Texture2D>          m_texture2D;
	uint32_t                         m_width = 0;
	uint32_t                         m_height = 0;
	uint32_t                         m_mipCount = 0;
	PixelFormat                      m_format;
};
//=============================================================================
class RenderTargetD3D11 final
{
public:
	RenderTargetD3D11(uint32_t width, uint32_t height, TextureD3D11* texture) :
		m_texture(texture)
	{
		auto format = EnumToD3D11(texture->GetFormat());
		auto rtv_desc = CD3D11_RENDER_TARGET_VIEW_DESC(D3D11_RTV_DIMENSION_TEXTURE2D, format);
		d3d11Device->CreateRenderTargetView(texture->GetD3D11Texture2D().Get(), &rtv_desc, m_renderTargetView.GetAddressOf());

		auto tex_desc = CD3D11_TEXTURE2D_DESC(DXGI_FORMAT_D24_UNORM_S8_UINT, width, height, 1, 1, D3D11_BIND_DEPTH_STENCIL);
		d3d11Device->CreateTexture2D(&tex_desc, NULL, m_depthStencilTexture.GetAddressOf());

		auto dsv_desc = CD3D11_DEPTH_STENCIL_VIEW_DESC(D3D11_DSV_DIMENSION_TEXTURE2D, tex_desc.Format);
		d3d11Device->CreateDepthStencilView(m_depthStencilTexture.Get(), &dsv_desc, m_depthStencilView.GetAddressOf());
	}

	const auto& GetD3D11RenderTargetView() const { return m_renderTargetView; }
	const auto& GetD3D11DepthStencilView() const { return m_depthStencilView; }
	auto GetTexture() const { return m_texture; }

private:
	ComPtr<ID3D11Texture2D>        m_depthStencilTexture;
	ComPtr<ID3D11RenderTargetView> m_renderTargetView;
	ComPtr<ID3D11DepthStencilView> m_depthStencilView;
	TextureD3D11*                  m_texture = nullptr;
};
//=============================================================================
void CreateMainRenderTarget(uint32_t width, uint32_t height)
{
	ComPtr<ID3D11Texture2D> backbuffer;
	d3d11SwapChain->GetBuffer(0, IID_PPV_ARGS(backbuffer.GetAddressOf()));

	backbufferTexture = new TextureD3D11(width, height, PixelFormat::RGBA8UNorm, backbuffer);
	mainRenderTarget = new RenderTargetD3D11(width, height, backbufferTexture);
}
//=============================================================================
void DestroyMainRenderTarget()
{
	delete backbufferTexture;
	delete mainRenderTarget;
	backbufferTexture = nullptr;
	mainRenderTarget = nullptr;
}
//=============================================================================
bool InitializeRender(HWND hwnd, uint16_t wndWidth, uint16_t wndHeight)
{
	windowWidth = wndWidth;
	windowHeight = wndHeight;

	// Select suitable graphics adapter and create device, context, and swap chain
	{
		ComPtr<IDXGIFactory6> dxgi_factory;
		HRESULT hResult = CreateDXGIFactory1(IID_PPV_ARGS(dxgi_factory.GetAddressOf()));
		if (FAILED(hResult))
		{
			Fatal("Failed to create DXGI Factory!");
			return false;
		}

		IDXGIAdapter1* adapter;
		auto gpu_preference = DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE;
		hResult = dxgi_factory->EnumAdapterByGpuPreference(0, gpu_preference, IID_PPV_ARGS(&adapter));
		if (FAILED(hResult))
		{
			Fatal("Failed to enumerate graphics adapter!");
			return false;
		}

		DXGI_SWAP_CHAIN_DESC d3d11SwapChainDesc = {};
		d3d11SwapChainDesc.BufferCount = 2;
		d3d11SwapChainDesc.BufferDesc.Width = windowWidth;
		d3d11SwapChainDesc.BufferDesc.Height = windowHeight;
		d3d11SwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		d3d11SwapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
		d3d11SwapChainDesc.BufferDesc.RefreshRate.Denominator = 5;
		d3d11SwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
		d3d11SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		d3d11SwapChainDesc.OutputWindow = hwnd;
		d3d11SwapChainDesc.SampleDesc.Count = 1;
		d3d11SwapChainDesc.SampleDesc.Quality = 0;
		d3d11SwapChainDesc.Windowed = TRUE;
		d3d11SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

#if defined(_DEBUG)
		UINT flags = D3D11_CREATE_DEVICE_DEBUG;
#else
		UINT flags = 0;
#endif

		hResult = D3D11CreateDeviceAndSwapChain(adapter, D3D_DRIVER_TYPE_UNKNOWN, NULL, flags, NULL, 0,
			D3D11_SDK_VERSION, &d3d11SwapChainDesc, d3d11SwapChain.GetAddressOf(), d3d11Device.GetAddressOf(),
			NULL, d3d11DeviceContext.GetAddressOf());
		if (FAILED(hResult))
		{
			Fatal("Failed to create D3D11 device and swap chain!");
			adapter->Release();
			return false;
		}
	}
		

#if defined(_DEBUG)
	ComPtr<ID3D11InfoQueue> info_queue;
	d3d11Device->QueryInterface(IID_PPV_ARGS(info_queue.GetAddressOf()));

	info_queue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
	info_queue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
	info_queue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_INFO, true);
	info_queue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_MESSAGE, true);
	info_queue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_WARNING, true);
#endif

	CreateMainRenderTarget(windowWidth, windowHeight);
	render::SetRenderTarget(nullptr, 0);

	// Create Framebuffer Render Target
	{
		ID3D11Texture2D* d3d11FrameBuffer;
		HRESULT hResult = d3d11SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&d3d11FrameBuffer);
		if (FAILED(hResult))
		{
			Fatal("Failed to get back buffer from Swap Chain!");
			return false;
		}

		hResult = d3d11Device->CreateRenderTargetView(d3d11FrameBuffer, 0, &d3d11FrameBufferView);		
		d3d11FrameBuffer->Release();
		if (FAILED(hResult))
		{
			Fatal("Failed to create Render Target View for back buffer!");
			return false;
		}
	}
	
	// Create Vertex Shader
	{
		ID3DBlob* shaderCompileErrorsBlob;
		HRESULT hResult = D3DCompileFromFile(L"shaders.hlsl", nullptr, nullptr, "vs_main", "vs_5_0", 0, 0, &vsBlob, &shaderCompileErrorsBlob);
		if (FAILED(hResult))
		{
			const char* errorString = NULL;
			if (hResult == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
				errorString = "Could not compile shader; file not found";
			else if (shaderCompileErrorsBlob) {
				errorString = (const char*)shaderCompileErrorsBlob->GetBufferPointer();
				shaderCompileErrorsBlob->Release();
			}
			return false;
		}

		hResult = d3d11Device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &vertexShader);
		assert(SUCCEEDED(hResult));
	}
	// Create Pixel Shader
	{
		ID3DBlob* psBlob;
		ID3DBlob* shaderCompileErrorsBlob;
		HRESULT hResult = D3DCompileFromFile(L"shaders.hlsl", nullptr, nullptr, "ps_main", "ps_5_0", 0, 0, &psBlob, &shaderCompileErrorsBlob);
		if (FAILED(hResult))
		{
			const char* errorString = NULL;
			if (hResult == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
				errorString = "Could not compile shader; file not found";
			else if (shaderCompileErrorsBlob) {
				errorString = (const char*)shaderCompileErrorsBlob->GetBufferPointer();
				shaderCompileErrorsBlob->Release();
			}
			return false;
		}

		hResult = d3d11Device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &pixelShader);
		assert(SUCCEEDED(hResult));
		psBlob->Release();
	}

	// Create Input Layout
	{
		D3D11_INPUT_ELEMENT_DESC inputElementDesc[] =
		{
			{ "POS", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEX", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
		};

		HRESULT hResult = d3d11Device->CreateInputLayout(inputElementDesc, ARRAYSIZE(inputElementDesc), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &inputLayout);
		assert(SUCCEEDED(hResult));
		vsBlob->Release();
	}

	// Create Vertex Buffer
	{
		float vertexData[] = { // x, y, u, v
			-0.5f,  0.5f, 0.f, 0.f,
			0.5f, -0.5f, 1.f, 1.f,
			-0.5f, -0.5f, 0.f, 1.f,
			-0.5f,  0.5f, 0.f, 0.f,
			0.5f,  0.5f, 1.f, 0.f,
			0.5f, -0.5f, 1.f, 1.f
		};
		stride = 4 * sizeof(float);
		numVerts = sizeof(vertexData) / stride;
		offset = 0;

		D3D11_BUFFER_DESC vertexBufferDesc = {};
		vertexBufferDesc.ByteWidth = sizeof(vertexData);
		vertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
		vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

		D3D11_SUBRESOURCE_DATA vertexSubresourceData = { vertexData };

		HRESULT hResult = d3d11Device->CreateBuffer(&vertexBufferDesc, &vertexSubresourceData, &vertexBuffer);
		assert(SUCCEEDED(hResult));
	}

	// Create Sampler State
	D3D11_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	samplerDesc.BorderColor[0] = 1.0f;
	samplerDesc.BorderColor[1] = 1.0f;
	samplerDesc.BorderColor[2] = 1.0f;
	samplerDesc.BorderColor[3] = 1.0f;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	d3d11Device->CreateSamplerState(&samplerDesc, &samplerState);

	// Load Image
	{
		std::vector<uint8_t> imageData(16*16*4);
		for (size_t y = 0; y < 16; y++)
		{
			for (size_t x = 0; x < 16; x++)
			{
				size_t index = (y * 16 + x) * 4;
				imageData[index + 0] = (x % 2 == y % 2) ? 255 : 100; // R
				imageData[index + 1] = (x % 2 == y % 2) ? 255 : 100; // G
				imageData[index + 2] = (x % 2 == y % 2) ? 255 : 100; // B
				imageData[index + 3] = 255; // A
			}
		}

		int texBytesPerRow = 4 * 16;

		// Create Texture
		D3D11_TEXTURE2D_DESC textureDesc = {};
		textureDesc.Width = 16;
		textureDesc.Height = 16;
		textureDesc.MipLevels = 1;
		textureDesc.ArraySize = 1;
		textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.Usage = D3D11_USAGE_IMMUTABLE;
		textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

		D3D11_SUBRESOURCE_DATA textureSubresourceData = {};
		textureSubresourceData.pSysMem = imageData.data();
		textureSubresourceData.SysMemPitch = texBytesPerRow;

		d3d11Device->CreateTexture2D(&textureDesc, &textureSubresourceData, &texture);
		d3d11Device->CreateShaderResourceView(texture, nullptr, &textureView);
	}

	return true;
}
//=============================================================================
void ShutdownRender()
{
	DestroyMainRenderTarget();
}
//=============================================================================
void RenderResize(uint16_t wndWidth, uint16_t wndHeight)
{
	windowWidth = wndWidth;
	windowHeight = wndHeight;

	DestroyMainRenderTarget();
	d3d11SwapChain->ResizeBuffers(0, (UINT)windowWidth, (UINT)windowHeight, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
	CreateMainRenderTarget(windowWidth, windowHeight);
	render::SetRenderTarget(nullptr, 0); // TODO: do it when nullptr was before

	if (!viewport.has_value())
		viewport_dirty = true;
}
//=============================================================================
void RenderSwap()
{
	FLOAT backgroundColor[4] = { 0.1f, 0.2f, 0.6f, 1.0f };
	d3d11DeviceContext->ClearRenderTargetView(d3d11FrameBufferView, backgroundColor);

	D3D11_VIEWPORT viewport = { 0.0f, 0.0f, (FLOAT)(windowWidth), (FLOAT)(windowHeight), 0.0f, 1.0f };
	d3d11DeviceContext->RSSetViewports(1, &viewport);

	d3d11DeviceContext->OMSetRenderTargets(1, &d3d11FrameBufferView, nullptr);

	d3d11DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	d3d11DeviceContext->IASetInputLayout(inputLayout);

	d3d11DeviceContext->VSSetShader(vertexShader, nullptr, 0);
	d3d11DeviceContext->PSSetShader(pixelShader, nullptr, 0);

	d3d11DeviceContext->PSSetShaderResources(0, 1, &textureView);
	d3d11DeviceContext->PSSetSamplers(0, 1, &samplerState);

	d3d11DeviceContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);

	d3d11DeviceContext->Draw(numVerts, 0);

	d3d11SwapChain->Present(1, 0);
}
//=============================================================================
void render::SetRenderTarget(const RenderTarget** render_target, size_t count)
{
	if (count == 0)
	{
		d3d11DeviceContext->OMSetRenderTargets(1, mainRenderTarget->GetD3D11RenderTargetView().GetAddressOf(),
			mainRenderTarget->GetD3D11DepthStencilView().Get());

		renderTargets = { mainRenderTarget };

		if (!viewport.has_value())
			viewportDirty = true;

		return;
	}
	todo
}
//=============================================================================
#endif // _WIN32