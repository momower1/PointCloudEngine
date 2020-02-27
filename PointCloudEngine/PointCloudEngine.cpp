#include "PointCloudEngine.h"

// Variables for window creation and global access
std::wstring executablePath;
std::wstring executableDirectory;
HRESULT hr;
HWND hwnd = NULL;
LPCTSTR WndClassName = L"PointCloudEngine";
double dt = 0;
Timer timer;
Scene scene;
Settings* settings;
Camera* camera;
Shader* textShader;
Shader* splatShader;
Shader* pointShader;
Shader* waypointShader;
Shader* octreeCubeShader;
Shader* octreeSplatShader;
Shader* octreeClusterShader;
Shader* octreeComputeShader;
Shader* octreeComputeVSShader;
Shader* blendingShader;
Shader* gammaCorrectionShader;
Shader* textureConversionShader;

// DirectX11 interface objects
IDXGISwapChain* swapChain;		                // Change between front and back buffer
ID3D11Device* d3d11Device;		                // GPU
ID3D11DeviceContext* d3d11DevCon;		        // (multi-threaded) rendering
ID3D11RenderTargetView* renderTargetView;		// 2D texture (backbuffer) -> output merger
ID3D11Texture2D* backBufferTexture;
ID3D11UnorderedAccessView* backBufferTextureUAV;
ID3D11DepthStencilView* depthStencilView;
ID3D11Texture2D* depthStencilTexture;
ID3D11ShaderResourceView* depthTextureSRV;
ID3D11DepthStencilState* depthStencilState;     // Standard depth/stencil state for 3d rendering
ID3D11BlendState* blendState;                   // Blend state that is used for transparency
ID3D11RasterizerState* rasterizerState;		    // Encapsulates settings for the rasterizer stage of the pipeline
		
// Lighting buffer
ID3D11Buffer* lightingConstantBuffer = NULL;
LightingConstantBuffer lightingConstantBufferData;

// For splat blending
ID3D11Texture2D* blendingDepthTexture;
ID3D11DepthStencilView* blendingDepthView;
ID3D11ShaderResourceView* blendingDepthTextureSRV;
ID3D11BlendState* additiveBlendState;
ID3D11DepthStencilState* disabledDepthStencilState;

// This is used to unbind buffers and views from the shaders
ID3D11Buffer* nullBuffer[1] = { NULL };
ID3D11UnorderedAccessView* nullUAV[1] = { NULL };
ID3D11ShaderResourceView* nullSRV[1] = { NULL };

bool OpenFileDialog(const wchar_t *filter, std::wstring& outFilename)
{
	// Show windows explorer open file dialog
	wchar_t filename[MAX_PATH];
	OPENFILENAMEW openFileName;
	ZeroMemory(&openFileName, sizeof(OPENFILENAMEW));
	openFileName.lStructSize = sizeof(OPENFILENAMEW);
	openFileName.hwndOwner = hwnd;
	openFileName.lpstrFilter = filter;
	openFileName.lpstrFile = filename;
	openFileName.lpstrFile[0] = L'\0';
	openFileName.nMaxFile = MAX_PATH;
	openFileName.lpstrTitle = L"Select a file to open!";
	openFileName.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
	openFileName.nFilterIndex = 1;

	if (GetOpenFileNameW(&openFileName))
	{
		outFilename = filename;
		return true;
	}

	return false;
}

void ErrorMessageOnFail(HRESULT hr, std::wstring message, std::wstring file, int line)
{
	if (FAILED(hr))
	{
		// Display the error code and message as well as the line and file of the error
		_com_error error(hr);
		std::wstringstream headerStream, messageStream;
		std::wstring filename = file.substr(file.find_last_of(L"/\\") + 1);
		headerStream << L"Error 0x" << std::hex << hr;
		messageStream << message << L"\n\n" << error.ErrorMessage() << L" in " << filename << " at line " << line;
		std::wstring header = std::wstring(headerStream.str());
		message = std::wstring(messageStream.str());
		MessageBox(hwnd, message.c_str(), header.c_str(), MB_ICONERROR | MB_APPLMODAL);
	}
}

bool LoadPointcloudFile(std::vector<Vertex>& outVertices, Vector3& outBoundingCubePosition, float& outBoundingCubeSize, const std::wstring& pointcloudFile)
{
	try
	{
		struct PointcloudVertex
		{
			// Stores the .pointcloud vertices
			Vector3 position;
			char normal[3];
			unsigned char color[3];
		};

		// Try to load the point cloud from the file
		// This file has a header with the bounding cube position and size followed by the length of the vertex array
		// Then the position, 8bit normal and 8bit rgb color of each vertex is stored in binary data
		std::ifstream file(pointcloudFile, std::ios::in | std::ios::binary);

		// Load the bounding cube position and size
		file.read((char*)&outBoundingCubePosition, sizeof(Vector3));
		file.read((char*)&outBoundingCubeSize, sizeof(float));

		// Load the size of the vertices vector
		UINT vertexCount;
		file.read((char*)&vertexCount, sizeof(UINT));

		// Read the binary data directly into the vertices vector
		std::vector<PointcloudVertex> pointcloudVertices = std::vector<PointcloudVertex>(vertexCount);
		file.read((char*)pointcloudVertices.data(), vertexCount * sizeof(PointcloudVertex));

		// Convert to the required vertex format
		outVertices = std::vector<Vertex>(vertexCount);

		for (UINT i = 0; i < vertexCount; i++)
		{
			outVertices[i].position = pointcloudVertices[i].position;
			outVertices[i].normal.x = pointcloudVertices[i].normal[0] / 127.0f;
			outVertices[i].normal.y = pointcloudVertices[i].normal[1] / 127.0f;
			outVertices[i].normal.z = pointcloudVertices[i].normal[2] / 127.0f;
			outVertices[i].color[0] = pointcloudVertices[i].color[0];
			outVertices[i].color[1] = pointcloudVertices[i].color[1];
			outVertices[i].color[2] = pointcloudVertices[i].color[2];
		}
	}
	catch (const std::exception& e)
	{
		return false;
	}

	return true;
}

void SaveScreenshotToFile()
{
	// Save the texture to the hard drive
	CreateDirectory((executableDirectory + L"/Screenshots").c_str(), NULL);
	hr = SaveWICTextureToFile(d3d11DevCon, backBufferTexture, GUID_ContainerFormatPng, (executableDirectory + L"/Screenshots/" + std::to_wstring(time(0)) + L".png").c_str());
	ERROR_MESSAGE_ON_FAIL(hr, NAMEOF(SaveWICTextureToFile) + L" failed in " + NAMEOF(SaveScreenshotToFile));

	if (SUCCEEDED(hr))
	{
		// Make a successful sound
		Beep(750, 75);
		Beep(1000, 150);
	}
}

void SetFullscreen(bool fullscreen)
{
	hr = swapChain->SetFullscreenState(fullscreen, NULL);
	ERROR_MESSAGE_ON_FAIL(hr, NAMEOF(swapChain->SetFullscreenState) + L" failed!");
}

void DrawBlended(UINT vertexCount, ID3D11Buffer* constantBuffer, const void* constantBufferData, int &useBlending)
{
	// Draw with blending
	// Before this is called all the shaders, buffers and resources have to be set already!
	// Draw only the depth to the depth texture, don't draw any color
	d3d11DevCon->ClearDepthStencilView(blendingDepthView, D3D11_CLEAR_DEPTH, 1.0f, 0);
	d3d11DevCon->OMSetRenderTargets(0, NULL, blendingDepthView);
	d3d11DevCon->Draw(vertexCount, 0);

	// Draw again but this time with the actual depth buffer, render target and blending
	useBlending = true;
	d3d11DevCon->UpdateSubresource(constantBuffer, 0, NULL, constantBufferData, 0, 0);
	d3d11DevCon->OMSetRenderTargets(1, &renderTargetView, depthStencilView);

	// Set a different blend state
	d3d11DevCon->OMSetBlendState(additiveBlendState, NULL, 0xffffffff);

	// Bind this depth texture to the shader
	d3d11DevCon->PSSetShaderResources(0, 1, &blendingDepthTextureSRV);

	// Disable depth test to make sure that all the overlapping splats are blended together
	d3d11DevCon->OMSetDepthStencilState(disabledDepthStencilState, 0);

	// Draw again only adding the colors and weights of the overlapping splats together
	d3d11DevCon->Draw(vertexCount, 0);

	// Unbind shader resources
	d3d11DevCon->PSSetShaderResources(0, 1, nullSRV);

	// Remove the depth stencil view from the render target in order to make it accessable by the pixel shader (also set the back buffer UAV)
	d3d11DevCon->OMSetRenderTargetsAndUnorderedAccessViews(0, NULL, NULL, 1, 1, &backBufferTextureUAV, NULL);
	d3d11DevCon->VSSetShader(blendingShader->vertexShader, NULL, 0);
	d3d11DevCon->GSSetShader(blendingShader->geometryShader, NULL, 0);
	d3d11DevCon->PSSetShader(blendingShader->pixelShader, NULL, 0);

	// Use pixel shader to divide the color sum by the weight sum of overlapping splats in each pixel, also remove background color
	d3d11DevCon->Draw(1, 0);

	// Unbind shader resources
	d3d11DevCon->VSSetShader(NULL, NULL, 0);
	d3d11DevCon->GSSetShader(NULL, NULL, 0);
	d3d11DevCon->PSSetShader(NULL, NULL, 0);

	// Reset to the defaults
	d3d11DevCon->OMSetRenderTargetsAndUnorderedAccessViews(1, &renderTargetView, depthStencilView, 1, 1, nullUAV, NULL);
	d3d11DevCon->OMSetDepthStencilState(depthStencilState, 0);
	d3d11DevCon->OMSetBlendState(blendState, NULL, 0xffffffff);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    // Save the executable directory path
    wchar_t buffer [MAX_PATH];
    GetModuleFileNameW(NULL, buffer, MAX_PATH);
    executablePath = std::wstring(buffer);
    executableDirectory = executablePath.substr(0, executablePath.find_last_of(L"\\/"));

    // Load the settings
    settings = new Settings();

	// Initialize the COM interface
	hr = CoInitialize(NULL);
	ERROR_MESSAGE_ON_FAIL(hr, NAMEOF(CoInitialize) + L" failed!");

	InitializeWindow(hInstance, nShowCmd);
	InitializeRenderingResources();
	InitializeScene();

	Messageloop();
	ReleaseObjects();
	return 0;
}

void InitializeWindow(HINSTANCE hInstance, int ShowWnd)
{
	/*
	int ShowWnd - How the window should be displayed.Some common commands are SW_SHOWMAXIMIZED, SW_SHOW, SW_SHOWMINIMIZED.
	int width - Width of the window in pixels
	int height - Height of the window in pixels
	bool windowed - False if the window is fullscreen and true if the window is windowed
	*/

	WNDCLASSEX wc;
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;		            // Redraw when the window is moved or changed size
	wc.lpfnWndProc = WindowProc;		                // lpfnWndProc is a pointer to the function we want to process the windows messages
	wc.cbClsExtra = NULL;		                        // cbClsExtra is the number of extra bytes allocated after WNDCLASSEX.
	wc.cbWndExtra = NULL;		                        // cbWndExtra specifies the number of bytes allocated after the windows instance.
	wc.hInstance = hInstance;		                    // Handle to the current application, GetModuleHandle() function can be used to get the current window application by passing NUll to its 1 parameter
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);		    // Cursor icon
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 2);		// Colors the background
	wc.lpszMenuName = NULL;		                        // Name to the menu that is attached to our window. we don't have one so we put NULL
	wc.lpszClassName = WndClassName;		            // Name the class here
	wc.hIcon = wc.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON));

	if (!RegisterClassEx(&wc))
	{
		ERROR_MESSAGE(NAMEOF(RegisterClassEx) + L" failed!");
	}

	// Load the menu that will be shown below the title bar
	HMENU menu = LoadMenu(hInstance, MAKEINTRESOURCE(IDR_MENU));

	// Create window with extended styles like WS_EX_ACCEPTFILES, WS_EX_APPWINDOW, WS_EX_CONTEXTHELP, WS_EX_TOOLWINDOW
	hwnd = CreateWindowEx(NULL, WndClassName, L"PointCloudEngine", WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT, settings->resolutionX, settings->resolutionY, NULL, menu, hInstance, NULL);

	if (!hwnd)
	{
		ERROR_MESSAGE(NAMEOF(CreateWindowEx) + L" failed!");
	}

	ShowWindow(hwnd, ShowWnd);
	UpdateWindow(hwnd);
    Input::Initialize(hwnd);
}

void InitializeRenderingResources()
{
	DXGI_MODE_DESC bufferDesc;																	// Describe the backbuffer
	ZeroMemory(&bufferDesc, sizeof(DXGI_MODE_DESC));											// Clear everything for safety
	bufferDesc.Width = settings->resolutionX;													// Resolution X
	bufferDesc.Height = settings->resolutionY;													// Resolution Y
	bufferDesc.RefreshRate.Numerator = 144;														// Hertz
	bufferDesc.RefreshRate.Denominator = 1;
	bufferDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;											// Describes the display format
	bufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;							// Describes the order in which the rasterizer renders - not used since we use double buffering
	bufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;											// Descibes how window scaling is handled

	DXGI_SWAP_CHAIN_DESC swapChainDesc;															// Describe the spaw chain
	ZeroMemory(&swapChainDesc, sizeof(DXGI_SWAP_CHAIN_DESC));
	swapChainDesc.BufferDesc = bufferDesc;
	swapChainDesc.SampleDesc.Count = 1;															// Possibly 2x, 4x 8x Multisampling -> Smooth choppiness in edges and lines
    swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_UNORDERED_ACCESS;
	swapChainDesc.BufferCount = 1;																// 1 for double buffering, 2 for triple buffering and so on
	swapChainDesc.OutputWindow = hwnd;
	swapChainDesc.Windowed = settings->windowed;												// Fullscreen might freeze the programm -> set windowed before exit
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;										// Let display driver decide what to do when swapping buffers

	// Create a factory that is used to find out information about the available GPUs
	IDXGIFactory* dxgiFactory = NULL;
	hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)(&dxgiFactory));
	ERROR_MESSAGE_ON_FAIL(hr, NAMEOF(CreateDXGIFactory) + L" failed!");

	// Store all the adapters and their descriptions
	std::vector<IDXGIAdapter*> adapters;
	std::vector<DXGI_ADAPTER_DESC> adapterDescriptions;
	
	// Query for all the available adapters (includes hardware and software)
	for (UINT adapterIndex = 0; true; adapterIndex++)
	{
		IDXGIAdapter* adapter = NULL;

		if (dxgiFactory->EnumAdapters(adapterIndex, &adapter) != DXGI_ERROR_NOT_FOUND)
		{
			DXGI_ADAPTER_DESC adapterDesc;
			adapter->GetDesc(&adapterDesc);
			adapterDescriptions.push_back(adapterDesc);
			adapters.push_back(adapter);
		}
		else
		{
			break;
		}
	}

	// Find the adapter with the most available video memory (makes sure that e.g. laptop does not use integrated graphics)
	UINT adapterWithMostDedicatedVideoMemory = 0;

	for (UINT i = 1; i < adapters.size(); i++)
	{
		if (adapterDescriptions[i].DedicatedVideoMemory > adapterDescriptions[adapterWithMostDedicatedVideoMemory].DedicatedVideoMemory)
		{
			adapterWithMostDedicatedVideoMemory = i;
		}
	}

	// Use this adapter for the device creation
	IDXGIAdapter *adapterToUse = adapters[adapterWithMostDedicatedVideoMemory];

#ifdef _DEBUG
    UINT flags = D3D11_CREATE_DEVICE_DEBUG;
#else
	UINT flags = 0;
#endif

	// Create device and swap chain
	hr = D3D11CreateDeviceAndSwapChain(adapterToUse, D3D_DRIVER_TYPE_UNKNOWN, NULL, flags, NULL, 0, D3D11_SDK_VERSION, &swapChainDesc, &swapChain, &d3d11Device, NULL, &d3d11DevCon);
	ERROR_MESSAGE_ON_FAIL(hr, NAMEOF(D3D11CreateDeviceAndSwapChain) + L" failed!");

	// Release no longer needed resources
	for (auto it = adapters.begin(); it != adapters.end(); it++)
	{
		SAFE_RELEASE(*it);
	}

	SAFE_RELEASE(dxgiFactory);

	// Create backbuffer for the render target view
	hr = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBufferTexture);

	// Create render target view, will be sended to the output merger stage of the pipeline
	hr = d3d11Device->CreateRenderTargetView(backBufferTexture, NULL, &renderTargetView);		// NULL -> view accesses all subresources in mipmap level 0
	ERROR_MESSAGE_ON_FAIL(hr, NAMEOF(d3d11Device->CreateRenderTargetView) + L" failed!");

	// Create an unordered acces view in order to access and manipulate the texture in shaders
	D3D11_UNORDERED_ACCESS_VIEW_DESC backBufferTextureUAVDesc;
	ZeroMemory(&backBufferTextureUAVDesc, sizeof(backBufferTextureUAVDesc));
	backBufferTextureUAVDesc.Format = bufferDesc.Format;
	backBufferTextureUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;

	hr = d3d11Device->CreateUnorderedAccessView(backBufferTexture, &backBufferTextureUAVDesc, &backBufferTextureUAV);
	ERROR_MESSAGE_ON_FAIL(hr, NAMEOF(d3d11Device->CreateUnorderedAccessView) + L" failed for the " + NAMEOF(backBufferTextureUAV));

	// Depth/Stencil buffer description (needed for 3D Scenes + mirrors and such)
	D3D11_TEXTURE2D_DESC depthStencilTextureDesc;
	depthStencilTextureDesc.Width = settings->resolutionX;
	depthStencilTextureDesc.Height = settings->resolutionY;
	depthStencilTextureDesc.MipLevels = 1;
	depthStencilTextureDesc.ArraySize = 1;
	depthStencilTextureDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	depthStencilTextureDesc.SampleDesc.Count = 1;
	depthStencilTextureDesc.SampleDesc.Quality = 0;
	depthStencilTextureDesc.Usage = D3D11_USAGE_DEFAULT;
	depthStencilTextureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	depthStencilTextureDesc.CPUAccessFlags = 0;
	depthStencilTextureDesc.MiscFlags = 0;

	// Create the depth/stencil texture
	hr = d3d11Device->CreateTexture2D(&depthStencilTextureDesc, NULL, &depthStencilTexture);
	ERROR_MESSAGE_ON_FAIL(hr, NAMEOF(d3d11Device->CreateTexture2D) + L" failed!");

	// Create a shader resource view in order to bind the depth part of the texture to a shader
	D3D11_SHADER_RESOURCE_VIEW_DESC depthTextureSRVDesc;
	ZeroMemory(&depthTextureSRVDesc, sizeof(depthTextureSRVDesc));
	depthTextureSRVDesc.Format = DXGI_FORMAT_R32_FLOAT;
	depthTextureSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	depthTextureSRVDesc.Texture2D.MipLevels = 1;

	hr = d3d11Device->CreateShaderResourceView(depthStencilTexture, &depthTextureSRVDesc, &depthTextureSRV);
	ERROR_MESSAGE_ON_FAIL(hr, NAMEOF(d3d11Device->CreateShaderResourceView) + L" failed for the " + NAMEOF(depthTextureSRV));

	// Create Depth / Stencil View
	D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
	ZeroMemory(&depthStencilViewDesc, sizeof(depthStencilViewDesc));
	depthStencilViewDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;

	hr = d3d11Device->CreateDepthStencilView(depthStencilTexture, &depthStencilViewDesc, &depthStencilView);
	ERROR_MESSAGE_ON_FAIL(hr, NAMEOF(d3d11Device->CreateDepthStencilView) + L" failed!");

    // Depth / Stencil description with disabled incremental stencil buffer
    D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
    depthStencilDesc.DepthEnable = true;
    depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
    depthStencilDesc.StencilEnable = false;
    depthStencilDesc.StencilReadMask = 0xFF;
    depthStencilDesc.StencilWriteMask = 0xFF;
    depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_INCR_SAT;
    depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR_SAT;
    depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_INCR_SAT;
    depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
    depthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_NEVER;

    // Create depth stencil state
    hr = d3d11Device->CreateDepthStencilState(&depthStencilDesc, &depthStencilState);
	ERROR_MESSAGE_ON_FAIL(hr, NAMEOF(d3d11Device->CreateDepthStencilState) + L" failed!");

    // Create blend state for transparency
    D3D11_BLEND_DESC blendStateDesc;
    ZeroMemory(&blendStateDesc, sizeof(blendStateDesc));
    blendStateDesc.RenderTarget[0].BlendEnable = true;
    blendStateDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendStateDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendStateDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendStateDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendStateDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendStateDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendStateDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    hr = d3d11Device->CreateBlendState(&blendStateDesc, &blendState);
	ERROR_MESSAGE_ON_FAIL(hr, NAMEOF(d3d11Device->CreateBlendState) + L" failed!");

	// Describing the render state
	D3D11_RASTERIZER_DESC rasterizerDesc;
	ZeroMemory(&rasterizerDesc, sizeof(D3D11_RASTERIZER_DESC));
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
	rasterizerDesc.CullMode = D3D11_CULL_BACK;
    rasterizerDesc.FrontCounterClockwise = FALSE;
    rasterizerDesc.DepthBias = D3D11_DEFAULT_DEPTH_BIAS;
    rasterizerDesc.DepthBiasClamp = D3D11_DEFAULT_DEPTH_BIAS_CLAMP;
    rasterizerDesc.SlopeScaledDepthBias = D3D11_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    rasterizerDesc.DepthClipEnable = TRUE;
    rasterizerDesc.ScissorEnable = FALSE;
    rasterizerDesc.MultisampleEnable = TRUE;
    rasterizerDesc.AntialiasedLineEnable = TRUE;

	hr = d3d11Device->CreateRasterizerState(&rasterizerDesc, &rasterizerState);
	ERROR_MESSAGE_ON_FAIL(hr, NAMEOF(d3d11Device->CreateRasterizerState) + L" failed!");

	// Bind the rasterizer render state
	d3d11DevCon->RSSetState(rasterizerState);

	// Create resources for blending starting with a second depth buffer
	hr = d3d11Device->CreateTexture2D(&depthStencilTextureDesc, NULL, &blendingDepthTexture);
	ERROR_MESSAGE_ON_FAIL(hr, NAMEOF(d3d11Device->CreateTexture2D) + L" failed for the " + NAMEOF(blendingDepthTexture));

	// Create view for the blending depth buffer
	hr = d3d11Device->CreateDepthStencilView(blendingDepthTexture, &depthStencilViewDesc, &blendingDepthView);
	ERROR_MESSAGE_ON_FAIL(hr, NAMEOF(d3d11Device->CreateDepthStencilView) + L" failed for the " + NAMEOF(blendingDepthView));

	// Create a shader resource view in order to bind the depth to a shader
	hr = d3d11Device->CreateShaderResourceView(blendingDepthTexture, &depthTextureSRVDesc, &blendingDepthTextureSRV);
	ERROR_MESSAGE_ON_FAIL(hr, NAMEOF(d3d11Device->CreateShaderResourceView) + L" failed for the " + NAMEOF(blendingDepthTextureSRV));

	// Create a blend state that adds all the colors of the overlapping fragments together
	D3D11_BLEND_DESC additiveBlendStateDesc;
	ZeroMemory(&additiveBlendStateDesc, sizeof(additiveBlendStateDesc));
	additiveBlendStateDesc.RenderTarget[0].BlendEnable = true;
	additiveBlendStateDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
	additiveBlendStateDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
	additiveBlendStateDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	additiveBlendStateDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	additiveBlendStateDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
	additiveBlendStateDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	additiveBlendStateDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	hr = d3d11Device->CreateBlendState(&additiveBlendStateDesc, &additiveBlendState);
	ERROR_MESSAGE_ON_FAIL(hr, NAMEOF(d3d11Device->CreateBlendState) + L" failed for the " + NAMEOF(additiveBlendState));

	// Create a depth/stencil state with disabled depth testing for blending
	D3D11_DEPTH_STENCIL_DESC disabledDepthStencilStateDesc = depthStencilDesc;
	disabledDepthStencilStateDesc.DepthEnable = false;

	hr = d3d11Device->CreateDepthStencilState(&disabledDepthStencilStateDesc, &disabledDepthStencilState);
	ERROR_MESSAGE_ON_FAIL(hr, NAMEOF(d3d11Device->CreateDepthStencilState) + L" failed for the " + NAMEOF(disabledDepthStencilState));
}

// Main part of the program
int Messageloop()
{
	MSG msg;		// Structure of the windows message - holds the message`s information
	ZeroMemory(&msg, sizeof(MSG));		// Clear memory

	// While there is a message
	while (true)
	{
		// See if there is a windows message
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
				break;

			TranslateMessage(&msg);		// Translating like the keyboard's virtual keys to characters
			DispatchMessage(&msg);		// Sends the message to our windows procedure, WindowProc
		}
		else
		{
			// Run game code
			UpdateScene();
			DrawScene();
			GUI::Update();
		}
	}

	return msg.wParam;
}

// Check messages for events
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    Input::HandleMessage(msg, wParam, lParam);
	GUI::HandleMessage(msg, wParam, lParam);

	switch (msg)
	{
	    case WM_DESTROY:
        {
		    PostQuitMessage(0);
		    return 0;
        }
		case WM_COMMAND:
		{
			switch (wParam)
			{
				case ID_FILE_OPEN:
				{
					scene.OpenPointcloudFile();
					break;
				}
				case ID_FILE_GROUNDTRUTHRENDERER:
				{
					settings->useOctree = false;
					scene.LoadFile(settings->pointcloudFile);
					break;
				}
				case ID_FILE_OCTREERENDERER:
				{
					settings->useOctree = true;
					scene.LoadFile(settings->pointcloudFile);
					break;
				}
				case ID_FILE_SCREENSHOT:
				{
					SaveScreenshotToFile();
					break;
				}
				case ID_EDIT_SETTINGS:
				{
					ShellExecute(0, L"open", (executableDirectory + SETTINGS_FILENAME).c_str(), 0, 0, SW_SHOW);
					break;
				}
				case ID_HELP_README:
				{
					ShellExecute(0, L"open", (executableDirectory + L"/Readme.txt").c_str(), 0, 0, SW_SHOW);
					break;
				}
				case ID_HELP_WEBSITE:
				{
					ShellExecute(0, 0, L"https://github.com/momower1/PointCloudEngine", 0, 0, SW_SHOW);
					break;
				}
			}
		}
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);		//Takes care of all other messages
}

void InitializeScene()
{
    // Create and initialize the camera
    camera = new Camera();

    // Compile the shared shaders
    textShader = Shader::Create(L"Shader/Text.hlsl", true, true, true, false, Shader::textLayout, 3);
    splatShader = Shader::Create(L"Shader/Splat.hlsl", true, true, true, false, Shader::splatLayout, 3);
	pointShader = Shader::Create(L"Shader/Point.hlsl", true, true, true, false, Shader::splatLayout, 3);
	waypointShader = Shader::Create(L"Shader/Waypoint.hlsl", true, false, true, false, Shader::waypointLayout, 2);
    octreeCubeShader = Shader::Create(L"Shader/OctreeCube.hlsl", true, true, true, false, Shader::octreeLayout, 14);
    octreeSplatShader = Shader::Create(L"Shader/OctreeSplat.hlsl", true, true, true, false, Shader::octreeLayout, 14);
    octreeClusterShader = Shader::Create(L"Shader/OctreeCluster.hlsl", true, true, true, false, Shader::octreeLayout, 14);
    octreeComputeShader = Shader::Create(L"Shader/OctreeCompute.hlsl", false, false, false, true, NULL, 0);
    octreeComputeVSShader = Shader::Create(L"Shader/OctreeComputeVS.hlsl", true, false, false, false, NULL, 0);
	blendingShader = Shader::Create(L"Shader/Blending.hlsl", true, true, true, false, NULL, 0);
	gammaCorrectionShader = Shader::Create(L"Shader/GammaCorrection.hlsl", true, true, true, false, NULL, 0);
	textureConversionShader = Shader::Create(L"Shader/TextureConversion.hlsl", true, true, true, false, NULL, 0);

    // Load fonts
	TextRenderer::CreateSpriteFont(L"Arial", L"Assets/Arial.spritefont");
    TextRenderer::CreateSpriteFont(L"Consolas", L"Assets/Consolas.spritefont");
    TextRenderer::CreateSpriteFont(L"Times New Roman", L"Assets/Times New Roman.spritefont");

	// Create the constant buffer for the lighting
	D3D11_BUFFER_DESC lightingConstantBufferDesc;
	ZeroMemory(&lightingConstantBufferDesc, sizeof(lightingConstantBufferDesc));
	lightingConstantBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	lightingConstantBufferDesc.ByteWidth = sizeof(LightingConstantBuffer);
	lightingConstantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	hr = d3d11Device->CreateBuffer(&lightingConstantBufferDesc, NULL, &lightingConstantBuffer);
	ERROR_MESSAGE_ON_FAIL(hr, NAMEOF(d3d11Device->CreateBuffer) + L" failed for the " + NAMEOF(lightingConstantBuffer));

	scene.Initialize();
    timer.ResetElapsedTime();
}

void UpdateScene()
{
    Input::Update();

    timer.Tick([&]()
    {
        dt = timer.GetElapsedSeconds();
    });

    scene.Update(timer);
}

void DrawScene()
{
    // Bind the render target view to the output merger stage of the pipeline, also bind depth/stencil view as well
    d3d11DevCon->OMSetRenderTargets(1, &renderTargetView, depthStencilView);	// 1 since there is only 1 view

	// Clear out backbuffer to the updated color
	d3d11DevCon->ClearRenderTargetView(renderTargetView, (float*)&settings->backgroundColor);
	
	// Refresh the depth/stencil view
	d3d11DevCon->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	// Set the depth / stencil state
	d3d11DevCon->OMSetDepthStencilState(depthStencilState, 0);

	// Set the blend state, blend factor will become (1, 1, 1, 1) when passing NULL
	d3d11DevCon->OMSetBlendState(blendState, NULL, 0xffffffff);

	// Set the lighting constant buffer
	lightingConstantBufferData.useLighting = settings->useLighting;
	lightingConstantBufferData.lightIntensity = settings->lightIntensity;
	lightingConstantBufferData.ambient = settings->ambient;
	lightingConstantBufferData.diffuse = settings->diffuse;
	lightingConstantBufferData.specular = settings->specular;
	lightingConstantBufferData.specularExponent = settings->specularExponent;
	lightingConstantBufferData.backgroundColor = (Vector3)settings->backgroundColor;

	// Use a headlight or a constant light direction
	if (settings->useHeadlight)
	{
		lightingConstantBufferData.lightDirection = camera->GetForward();
	}
	else
	{
		lightingConstantBufferData.lightDirection = settings->lightDirection;
	}

	// Update the buffer
	d3d11DevCon->UpdateSubresource(lightingConstantBuffer, 0, NULL, &lightingConstantBufferData, 0, 0);

	// Set the buffer for the pixel shader
	d3d11DevCon->PSSetConstantBuffers(1, 1, &lightingConstantBuffer);

	// Calculates view and projection matrices and sets the viewport
	camera->PrepareDraw();

    // Draw scene
	scene.Draw();

	// Gamma correction is automatically applied in full screen mode, only apply it to the texture after presenting it to the screen (then screenshots will also be gamma corrected)
	// In window mode the gamma correction has to be done before presenting it to the screen
	BOOL fullscreen;
	IDXGIOutput* output = NULL;
	swapChain->GetFullscreenState(&fullscreen, &output);

	// Set the shader that will be used for the gamma correction
	d3d11DevCon->VSSetShader(gammaCorrectionShader->vertexShader, NULL, 0);
	d3d11DevCon->GSSetShader(gammaCorrectionShader->geometryShader, NULL, 0);
	d3d11DevCon->PSSetShader(gammaCorrectionShader->pixelShader, NULL, 0);
	d3d11DevCon->OMSetRenderTargetsAndUnorderedAccessViews(0, NULL, NULL, 1, 1, &backBufferTextureUAV, NULL);

	if (fullscreen)
	{
		// Present backbuffer to the screen
		hr = swapChain->Present(1, 0);
		ERROR_MESSAGE_ON_FAIL(hr, NAMEOF(swapChain->Present) + L" failed!");

		// Perform gamma correction
		d3d11DevCon->Draw(1, 0);
	}
	else
	{
		// Perform gamma correction
		d3d11DevCon->Draw(1, 0);

		// Present backbuffer to the screen
		hr = swapChain->Present(1, 0);
		ERROR_MESSAGE_ON_FAIL(hr, NAMEOF(swapChain->Present) + L" failed!");
	}

	d3d11DevCon->VSSetShader(NULL, NULL, 0);
	d3d11DevCon->GSSetShader(NULL, NULL, 0);
	d3d11DevCon->PSSetShader(NULL, NULL, 0);
	d3d11DevCon->OMSetRenderTargets(1, &renderTargetView, depthStencilView);
}

void ReleaseObjects()
{
	// Release scene objects
	scene.Release();

    // Delete settings (also saves them to the hard drive)
    SafeDelete(settings);
    SafeDelete(camera);

    // Release and delete shaders
    Shader::ReleaseAllShaders();
    TextRenderer::ReleaseAllSpriteFonts();

    // Release the COM (Component Object Model) objects
    SAFE_RELEASE(swapChain);
    SAFE_RELEASE(d3d11Device);
    SAFE_RELEASE(d3d11DevCon);
    SAFE_RELEASE(renderTargetView);
	SAFE_RELEASE(backBufferTexture);
	SAFE_RELEASE(backBufferTextureUAV);
    SAFE_RELEASE(depthStencilView);
    SAFE_RELEASE(depthStencilState);
    SAFE_RELEASE(depthStencilTexture);
	SAFE_RELEASE(depthTextureSRV);
    SAFE_RELEASE(rasterizerState);
	SAFE_RELEASE(lightingConstantBuffer);

	// Release resources for blending
	SAFE_RELEASE(blendingDepthTexture);
	SAFE_RELEASE(blendingDepthView);
	SAFE_RELEASE(blendingDepthTextureSRV);
	SAFE_RELEASE(additiveBlendState);
	SAFE_RELEASE(disabledDepthStencilState);
}
