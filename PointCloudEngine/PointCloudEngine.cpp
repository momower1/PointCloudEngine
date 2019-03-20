#include "PointCloudEngine.h"

// For the window creation
HRESULT hr;
Camera camera;
HWND hwnd = NULL;
LPCTSTR WndClassName = L"PointCloudEngine";
int resolutionX = 1280;
int resolutionY = 720;
int msaaCount = 1;
bool windowed = true;
double dt = 0;
Timer timer;
Scene scene;
Shader* textShader;
Shader* pointCloudShader;
Shader* pointCloudLODShader;

// DirectX11 interface objects
IDXGISwapChain* swapChain;		                // Change between front and back buffer
ID3D11Device* d3d11Device;		                // GPU
ID3D11DeviceContext* d3d11DevCon;		        // (multi-threaded) rendering
ID3D11RenderTargetView* renderTargetView;		// 2D texture (backbuffer) -> output merger
ID3D11DepthStencilView* depthStencilView;
ID3D11Texture2D* depthStencilBuffer;
ID3D11DepthStencilState* depthStencilState;     // Standard depth/stencil state for 3d rendering
ID3D11BlendState* blendState;                   // Blend state that is used for transparency
ID3D11RasterizerState* rasterizerState;		    // Encapsulates settings for the rasterizer stage of the pipeline
D3D11_VIEWPORT viewport;

void ErrorMessage(std::wstring message, std::wstring header, std::wstring file, int line, HRESULT hr)
{
    if (FAILED(hr))
    {
        _com_error error(hr);
        std::wstringstream headerStream, messageStream;
        std::wstring filename = file.substr(file.find_last_of(L"/\\") + 1);
        headerStream << L"Error 0x" << std::hex << hr << L" " << header;
        messageStream << message << L"\n\n" << error.ErrorMessage() << L" in " << filename << " at line " << line;
        header = std::wstring(headerStream.str());
        message = std::wstring(messageStream.str());
        MessageBox(hwnd, message.c_str(), header.c_str(), MB_ICONERROR | MB_APPLMODAL);
    }
}

void SafeRelease(ID3D11Resource *resource)
{
    if (resource != NULL)
    {
        resource->Release();
        resource = NULL;
    }
}

Vector3 GetPerpendicularVector (Vector3 &v)
{
    if (v.x != 0)
        return Vector3(-(v.y + v.z) / v.x, 1, 1);

    if (v.y != 0)
        return Vector3(1, -(v.x + v.z) / v.y, 1);

    if (v.z != 0)
        return Vector3(1, 1, -(v.x + v.y) / v.z);

    return Vector3::Zero;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	if (!InitializeWindow(hInstance, nShowCmd, resolutionX, resolutionY, true))
	{
        ErrorMessage(L"Window Initialization failed.", L"WinMain", __FILEW__, __LINE__);
		return 0;
	}

	if (!InitializeDirect3d11App(hInstance))
	{
        ErrorMessage(L"Direct3D Initialization failed.", L"WinMain", __FILEW__, __LINE__);
		return 0;
	}

	if (!InitializeScene())
	{
        ErrorMessage(L"Scene Initialization failed.", L"WinMain", __FILEW__, __LINE__);
		return 0;
	}

	Messageloop();
	ReleaseObjects();
	return 0;
}

bool InitializeWindow(HINSTANCE hInstance, int ShowWnd, int width, int height, bool windowed)
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
	wc.lpfnWndProc = WndProc;		                    // lpfnWndProc is a pointer to the function we want to process the windows messages
	wc.cbClsExtra = NULL;		                        // cbClsExtra is the number of extra bytes allocated after WNDCLASSEX.
	wc.cbWndExtra = NULL;		                        // cbWndExtra specifies the number of bytes allocated after the windows instance.
	wc.hInstance = hInstance;		                    // Handle to the current application, GetModuleHandle() function can be used to get the current window application by passing NUll to its 1 parameter
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);		    // Cursor icon
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 2);		// Colors the background
	wc.lpszMenuName = NULL;		                        // Name to the menu that is attached to our window. we don't have one so we put NULL
	wc.lpszClassName = WndClassName;		            // Name the class here
    wc.hIcon = wc.hIconSm = (HICON) LoadImage(NULL, L"Assets/Icon.ico", IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE | LR_SHARED);

	if (!RegisterClassEx(&wc))
	{
        ErrorMessage(L"Class Registration failed.", L"InitializeWindow", __FILEW__, __LINE__);
		return 1;
	}

	// Create window with extended styles like WS_EX_ACCEPTFILES, WS_EX_APPWINDOW, WS_EX_CONTEXTHELP, WS_EX_TOOLWINDOW
	hwnd = CreateWindowEx(NULL, WndClassName, L"PointCloudEngine", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, NULL, NULL, hInstance, NULL);

	if (!hwnd)
	{
        ErrorMessage(L"Window Creation failed.", L"InitializeWindow", __FILEW__, __LINE__);
		return 1;
	}

	ShowWindow(hwnd, ShowWnd);
	UpdateWindow(hwnd);
    Input::Initialize(hwnd);

	return true;
}

bool InitializeDirect3d11App(HINSTANCE hInstance)
{    
	DXGI_MODE_DESC bufferDesc;		                                        // Describe the backbuffer
	ZeroMemory(&bufferDesc, sizeof(DXGI_MODE_DESC));		                // Clear everything for safety
	bufferDesc.Width = resolutionX;		                                    // Resolution X
	bufferDesc.Height = resolutionY;		                                // Resolution Y
	bufferDesc.RefreshRate.Numerator = 144;		                            // Hertz
	bufferDesc.RefreshRate.Denominator = 1;
	bufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;		                    // Describes the display format. 32bit unsigned int for 8bit Color RGBA
	bufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;		// Describes the order in which the rasterizer renders - not used since we use double buffering
	bufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;		                // Descibes how window scaling is handled

	DXGI_SWAP_CHAIN_DESC swapChainDesc;		                                // Describe the spaw chain
	ZeroMemory(&swapChainDesc, sizeof(DXGI_SWAP_CHAIN_DESC));
	swapChainDesc.BufferDesc = bufferDesc;
	swapChainDesc.SampleDesc.Count = msaaCount;		                                // Multisampling -> Smooth choppiness in edges and lines
    swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;		    // Describing the access the cpu has to the surface of the back buffer
	swapChainDesc.BufferCount = 1;		                                    // 1 for double buffering, 2 for triple buffering and so on
	swapChainDesc.OutputWindow = hwnd;
	swapChainDesc.Windowed = windowed;		                                // Fullscreen might freeze the programm -> set windowed before exit
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;		            // Let display driver decide what to do when swapping buffers

	// Create device and swap chain
	hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, NULL, NULL, NULL, D3D11_SDK_VERSION, &swapChainDesc, &swapChain, &d3d11Device, NULL, &d3d11DevCon);
    ErrorMessage(L"D3D11CreateDeviceAndSwapChain failed.", L"InitializeDirect3d11App", __FILEW__, __LINE__, hr);

	ID3D11Texture2D* backBuffer;
	hr = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);		// Create backbuffer for the render target view

	// Create render target view, will be sended to the output merger stage of the pipeline
	hr = d3d11Device->CreateRenderTargetView(backBuffer, NULL, &renderTargetView);		// NULL -> view accesses all subresources in mipmap level 0
    ErrorMessage(L"CreateRenderTargetView failed.", L"InitializeDirect3d11App", __FILEW__, __LINE__, hr);
    SafeRelease(backBuffer);

	// Depth/Stencil buffer description (needed for 3D Scenes + mirrors and such)
	D3D11_TEXTURE2D_DESC depthStencilBufferDesc;
	depthStencilBufferDesc.Width = resolutionX;
	depthStencilBufferDesc.Height = resolutionY;
	depthStencilBufferDesc.MipLevels = 1;
	depthStencilBufferDesc.ArraySize = 1;
	depthStencilBufferDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthStencilBufferDesc.SampleDesc.Count = msaaCount;
	depthStencilBufferDesc.SampleDesc.Quality = 0;
	depthStencilBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	depthStencilBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthStencilBufferDesc.CPUAccessFlags = 0;
	depthStencilBufferDesc.MiscFlags = 0;

	// Create the depth/stencil view
	hr = d3d11Device->CreateTexture2D(&depthStencilBufferDesc, NULL, &depthStencilBuffer);
    ErrorMessage(L"CreateTexture2D failed.", L"InitializeDirect3d11App", __FILEW__, __LINE__, hr);

    // Depth / Stencil description
    D3D11_DEPTH_STENCIL_DESC depthStencilDesc;

    // Depth test parameters
    depthStencilDesc.DepthEnable = true;
    depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;

    // Stencil test parameters
    depthStencilDesc.StencilEnable = true;
    depthStencilDesc.StencilReadMask = 0xFF;
    depthStencilDesc.StencilWriteMask = 0xFF;

    // Stencil operations if pixel is front-facing
    depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
    depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

    // Stencil operations if pixel is back-facing
    depthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
    depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

    // Create depth stencil state
    hr = d3d11Device->CreateDepthStencilState(&depthStencilDesc, &depthStencilState);
    ErrorMessage(L"CreateDepthStencilState failed.", L"InitializeDirect3d11App", __FILEW__, __LINE__, hr);
    d3d11DevCon->OMSetDepthStencilState(depthStencilState, 0);

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
    ErrorMessage(L"CreateBlendState failed.", L"InitializeDirect3d11App", __FILEW__, __LINE__, hr);
    float blendFactor[] = {0, 0, 0, 0};
    UINT sampleMask = 0xffffffff;
    d3d11DevCon->OMSetBlendState(blendState, blendFactor, sampleMask);

    // Create Depth / Stencil View
    D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
    ZeroMemory(&depthStencilViewDesc, sizeof(depthStencilViewDesc));
    depthStencilViewDesc.Format = depthStencilBufferDesc.Format;
    depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
    depthStencilViewDesc.Texture2DMS.UnusedField_NothingToDefine = 0;

	hr = d3d11Device->CreateDepthStencilView(depthStencilBuffer, &depthStencilViewDesc, &depthStencilView);
    ErrorMessage(L"CreateDepthStencilView failed.", L"InitializeDirect3d11App", __FILEW__, __LINE__, hr);

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
    ErrorMessage(L"CreateRasterizerState failed.", L"InitializeDirect3d11App", __FILEW__, __LINE__, hr);

	// Bind the rasterizer render state
	d3d11DevCon->RSSetState(rasterizerState);

	return true;
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
			DispatchMessage(&msg);		// Sends the message to our windows procedure, WndProc
		}
		else
		{
			// Run game code
			UpdateScene();
			DrawScene();
		}
	}

	return msg.wParam;
}

// Check messages for events
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    Input::ProcessMessage(msg, wParam, lParam);

	switch (msg)
	{
	    case WM_DESTROY:
        {
		    PostQuitMessage(0);
		    return 0;
        }
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);		//Takes care of all other messages
}

bool InitializeScene()
{
	// Create the viewport
	ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	viewport.Width = resolutionX;
	viewport.Height = resolutionY;

    // Compile the shared shaders
    textShader = Shader::Create(L"Text.hlsl", true, true, true, Shader::textLayout, 3);
    pointCloudShader = Shader::Create(L"PointCloud.hlsl", true, true, true, Shader::pointCloudLayout, 3);
    pointCloudLODShader = Shader::Create(L"PointCloudLOD.hlsl", true, true, true, Shader::pointCloudLODLayout, 3);

    // Load fonts
    TextRenderer::CreateSpriteFont(L"Consolas", L"Assets/Consolas.spritefont");
    TextRenderer::CreateSpriteFont(L"Times New Roman", L"Assets/Times New Roman.spritefont");

	scene.Initialize();
    timer.ResetElapsedTime();

	return true;
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
	float backgroundColor[4] = { 0.75f, 0.75f, 1.0f, 1.0f };
	d3d11DevCon->ClearRenderTargetView(renderTargetView, backgroundColor);
	
	// Refresh the depth/stencil view
	d3d11DevCon->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    // Camera view and projection matrix
    camera.CalculateViewProjection(0.4f * XM_PI, (float)resolutionX / (float)resolutionY, 0.1f, 1000.0f);

    // Bind viewport to rasterization stage
    d3d11DevCon->RSSetViewports(1, &viewport);

    // Draw scene
    scene.Draw();

	// Present backbuffer to the screen
	hr = swapChain->Present(1, 0);
    ErrorMessage(L"SwapChain->Present failed.", L"DrawScene", __FILEW__, __LINE__, hr);
}

void ReleaseObjects()
{
    // Release and delete shaders
    Shader::ReleaseAllShaders();
    TextRenderer::ReleaseAllSpriteFonts();

    // Release scene objects
    scene.Release();

    // Release the COM (Component Object Model) objects
    swapChain->Release();
    d3d11Device->Release();
    d3d11DevCon->Release();
    renderTargetView->Release();
    depthStencilView->Release();
    depthStencilState->Release();
    rasterizerState->Release();

    SafeRelease(depthStencilBuffer);
}
