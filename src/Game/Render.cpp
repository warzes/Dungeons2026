#include "stdafx.h"
#if defined(_WIN32)
#include "Render.h"
#include "Core.h"
//=============================================================================
namespace
{
	uint16_t windowWidth{ 0 };
	uint16_t windowHeight{ 0 };
	ID3D11Device5*          d3d11Device{ nullptr };
	ID3D11DeviceContext4*   d3d11DeviceContext{ nullptr };
	IDXGISwapChain1*        d3d11SwapChain{ nullptr };
	ID3D11RenderTargetView* d3d11FrameBufferView{ nullptr };



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
bool InitializeRender(HWND hwnd, uint16_t wndWidth, uint16_t wndHeight)
{
	windowWidth = wndWidth;
	windowHeight = wndHeight;

	// Create D3D11 Device and Context
	{
		ID3D11Device* baseDevice;
		ID3D11DeviceContext* baseDeviceContext;
		D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };
		UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(_DEBUG)
		creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
		HRESULT hResult = D3D11CreateDevice(0, D3D_DRIVER_TYPE_HARDWARE,
			0, creationFlags,
			featureLevels, ARRAYSIZE(featureLevels),
			D3D11_SDK_VERSION, &baseDevice,
			0, &baseDeviceContext);
		if (FAILED(hResult))
		{
			Fatal("Failed to create D3D11 device and context!");
			return false;
		}
		// Get 5 interface of D3D11 Device and Context
		hResult = baseDevice->QueryInterface(__uuidof(ID3D11Device5), (void**)&d3d11Device);
		baseDevice->Release();
		if (FAILED(hResult))
		{
			Fatal("Failed to get ID3D11Device5 interface!");
			baseDeviceContext->Release();
			return false;
		}

		hResult = baseDeviceContext->QueryInterface(__uuidof(ID3D11DeviceContext4), (void**)&d3d11DeviceContext);
		baseDeviceContext->Release();
		if (FAILED(hResult))
		{
			Fatal("Failed to get ID3D11DeviceContext4 interface!");
			return false;
		}
	}
	

#if defined(_DEBUG)
	// Set up debug layer to break on D3D11 errors
	ID3D11Debug* d3dDebug = nullptr;
	d3d11Device->QueryInterface(__uuidof(ID3D11Debug), (void**)&d3dDebug);
	if (d3dDebug)
	{
		ID3D11InfoQueue* d3dInfoQueue = nullptr;
		if (SUCCEEDED(d3dDebug->QueryInterface(__uuidof(ID3D11InfoQueue), (void**)&d3dInfoQueue)))
		{
			d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
			d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
			d3dInfoQueue->Release();
		}
		d3dDebug->Release();
	}
#endif

	// Create Swap Chain
	{
		// Get DXGI Factory (needed to create Swap Chain)
		IDXGIFactory5* dxgiFactory;
		{
			IDXGIDevice1* dxgiDevice;
			HRESULT hResult = d3d11Device->QueryInterface(__uuidof(IDXGIDevice1), (void**)&dxgiDevice);
			if (FAILED(hResult))
			{
				Fatal("Failed to get IDXGIDevice1 from D3D11 device!");
				return false;
			}

			IDXGIAdapter* dxgiAdapter;
			hResult = dxgiDevice->GetAdapter(&dxgiAdapter);
			dxgiDevice->Release();
			if (FAILED(hResult))
			{
				Fatal("Failed to get IDXGIAdapter from DXGI device!");	
				return false;
			}

			DXGI_ADAPTER_DESC adapterDesc;
			dxgiAdapter->GetDesc(&adapterDesc);

			OutputDebugString(L"Graphics Device: ");
			OutputDebugString(adapterDesc.Description);

			hResult = dxgiAdapter->GetParent(__uuidof(IDXGIFactory5), (void**)&dxgiFactory);
			dxgiAdapter->Release();
			if (FAILED(hResult))
			{
				Fatal("Failed to get IDXGIFactory5 from DXGI adapter!");
				return false;
			}
		}

		DXGI_SWAP_CHAIN_DESC1 d3d11SwapChainDesc = {};
		d3d11SwapChainDesc.Width = 0; // use window width
		d3d11SwapChainDesc.Height = 0; // use window height
		d3d11SwapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
		d3d11SwapChainDesc.SampleDesc.Count = 1;
		d3d11SwapChainDesc.SampleDesc.Quality = 0;
		d3d11SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		d3d11SwapChainDesc.BufferCount = 2;
		d3d11SwapChainDesc.Scaling = DXGI_SCALING_STRETCH;
		d3d11SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		d3d11SwapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
		d3d11SwapChainDesc.Flags = 0;

		HRESULT hResult = dxgiFactory->CreateSwapChainForHwnd(d3d11Device, hwnd, &d3d11SwapChainDesc, 0, 0, &d3d11SwapChain);
		dxgiFactory->Release();
		if (FAILED(hResult))
		{
			Fatal("Failed to create DXGI Swap Chain!");
			return false;
		}
	}

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
	// Windows-specific rendering shutdown code
}
//=============================================================================
void RenderResize(uint16_t wndWidth, uint16_t wndHeight)
{
	windowWidth = wndWidth;
	windowHeight = wndHeight;

	d3d11DeviceContext->OMSetRenderTargets(0, 0, 0);
	d3d11FrameBufferView->Release();

	HRESULT res = d3d11SwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
	assert(SUCCEEDED(res));

	ID3D11Texture2D* d3d11FrameBuffer;
	res = d3d11SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&d3d11FrameBuffer);
	assert(SUCCEEDED(res));

	res = d3d11Device->CreateRenderTargetView(d3d11FrameBuffer, NULL, &d3d11FrameBufferView);
	assert(SUCCEEDED(res));
	d3d11FrameBuffer->Release();
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
#endif // _WIN32