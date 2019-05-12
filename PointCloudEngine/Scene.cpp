#include "Scene.h"

void Scene::Initialize()
{
    // Create the object for the point cloud
    pointCloud = Hierarchy::Create(L"PointCloud");

	// Create startup text
	TextRenderer* startupTextRenderer = new TextRenderer(TextRenderer::GetSpriteFont(L"Arial"), false);
	startupTextRenderer->text = L"Welcome to PointCloudEngine!\nThis engine renders .ply point cloud files by generating an octree.\nYou can change parameters (resolution, ...) in the settings file.\n\n\nPress [O] to open a .ply file.\nOnly x,y,z,nx,ny,nz,red,green,blue format is supported.";
	startupText = Hierarchy::Create(L"Startup Text");
	startupText->AddComponent(startupTextRenderer);
	startupText->transform->scale = Vector3(0.25, 0.35, 1);
	startupText->transform->position = Vector3(-0.95f, 0.4f, 0.5f);

    // Create loading text and hide it
    TextRenderer *loadingTextRenderer = new TextRenderer(TextRenderer::GetSpriteFont(L"Arial"), false);
    loadingTextRenderer->text = L"Loading...";
    loadingText = Hierarchy::Create(L"Loading Text");
    loadingText->AddComponent(loadingTextRenderer);
    loadingText->transform->scale = Vector3::Zero;
    loadingText->transform->position = Vector3(-0.5f, 0.25f, 0.5f);

    // Create text renderer to display properties, hide it first
    helpTextRenderer = new TextRenderer(TextRenderer::GetSpriteFont(L"Consolas"), false);
    helpText = Hierarchy::Create(L"Help Text");
    helpText->AddComponent(helpTextRenderer);
    helpText->transform->position = Vector3(-1, 1, 0.5f);
	helpText->transform->scale = Vector3::Zero;

	// Create fps text in top right corner
	fpsTextRenderer = new TextRenderer(TextRenderer::GetSpriteFont(L"Consolas"), false);
	fpsText = Hierarchy::Create(L"FPS Text");
	fpsText->AddComponent(fpsTextRenderer);
	fpsText->transform->position = Vector3(0.825f, 1, 0.5f);
	fpsText->transform->scale = 0.35f * Vector3::One;

	// Create the constant buffer for the lighting
	D3D11_BUFFER_DESC lightingConstantBufferDesc;
	ZeroMemory(&lightingConstantBufferDesc, sizeof(lightingConstantBufferDesc));
	lightingConstantBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	lightingConstantBufferDesc.ByteWidth = sizeof(LightingConstantBuffer);
	lightingConstantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	hr = d3d11Device->CreateBuffer(&lightingConstantBufferDesc, NULL, &lightingConstantBuffer);
	ERROR_MESSAGE_ON_FAIL(hr, NAMEOF(d3d11Device->CreateBuffer) + L" failed for the " + NAMEOF(lightingConstantBuffer));

    // Try to load the last plyfile
    DelayedLoadFile(settings->plyfile);
}

void Scene::Update(Timer &timer)
{
	// Screenshot on F9
	if (Input::GetKeyDown(Keyboard::F9))
	{
		SaveScreenshotToFile();
	}

    // Toggle help
    if (Input::GetKeyDown(Keyboard::H))
    {
        help = !help;
    }

	// Toggle lighting with L
	if (Input::GetKeyDown(Keyboard::L))
	{
		lightingConstantBufferData.useLighting = !lightingConstantBufferData.useLighting;
	}

	// Rotate the point cloud
	if (Input::GetKey(Keyboard::Space))
	{
		pointCloud->transform->rotation *= Quaternion::CreateFromYawPitchRoll(dt / 2, 0, 0);
	}

	// Set the sampling rate (minimal distance between two points) of the loaded point cloud
	if (Input::GetKey(Keyboard::Q))
	{
		settings->samplingRate = max(0.0001f, settings->samplingRate - dt * 0.01f);
	}
	else if (Input::GetKey(Keyboard::E))
	{
		settings->samplingRate += dt * 0.01f;
	}

    // Scale the point cloud by the value saved in the config file
    settings->scale = max(0.1f, settings->scale + Input::mouseScrollDelta);
    pointCloud->transform->scale = settings->scale * Vector3::One;

	// Select a camera position and rotation
	for (int i = 0; i < 6; i++)
	{
		if (Input::GetKeyDown((Keyboard::Keys)(Keyboard::F1 + i)))
		{
			camera->SetPosition(cameraPositions[i]);
			cameraPitch = cameraPitchYaws[i].x;
			cameraYaw = cameraPitchYaws[i].y;
		}
	}

    // Rotate camera with mouse, make sure that this doesn't happen with the accumulated input right after the file loaded
    if (timeSinceLoadFile > 0.1f)
    {
        cameraYaw += Input::mouseDelta.x;
        cameraPitch += Input::mouseDelta.y;
        cameraPitch = cameraPitch > XM_PI / 2.1f ? XM_PI / 2.1f : (cameraPitch < -XM_PI / 2.1f ? -XM_PI / 2.1f : cameraPitch);
        camera->SetRotationMatrix(Matrix::CreateFromYawPitchRoll(cameraYaw, cameraPitch, 0));
    }
    else
    {
        timeSinceLoadFile += dt;
    }

    // Speed up camera when pressing shift
    if (Input::GetKey(Keyboard::LeftShift))
    {
        cameraSpeed += 20 * dt;
    }
    else
    {
        cameraSpeed = 3;
    }

    // Move camera with WASD keys
    camera->TranslateRUF(cameraSpeed * dt * (Input::GetKey(Keyboard::D) - Input::GetKey(Keyboard::A)), 0, cameraSpeed * dt * (Input::GetKey(Keyboard::W) - Input::GetKey(Keyboard::S)));

	helpTextRenderer->text = L"[H] Toggle help\n";

    // Show help / controls
    if (help)
    {
        helpTextRenderer->text.append(L"[O] Open .ply file with (x,y,z,nx,ny,nz,red,green,blue) format\n");
		helpTextRenderer->text.append(L"[UP/DOWN] Increase/decrease splat resolution\n");
		helpTextRenderer->text.append(L"[N/V] Increase/decrease blending depth epsilon\n");
		helpTextRenderer->text.append(L"[E/Q] Increase/decrease sampling rate\n");
		helpTextRenderer->text.append(L"[BACKSPACE] Toggle CPU/GPU octree traversal\n");
		helpTextRenderer->text.append(L"[C] Toggle View Frustum & Visibility Culling\n");
		helpTextRenderer->text.append(L"[RIGHT/LEFT] Increase/decrease octree level\n");
		helpTextRenderer->text.append(L"[ENTER] Switch node view mode\n");
		helpTextRenderer->text.append(L"[SPACE] Rotate around y axis\n");
		helpTextRenderer->text.append(L"[F1-F6] Select camera position\n");
		helpTextRenderer->text.append(L"[MOUSE WHEEL] Scale\n");
		helpTextRenderer->text.append(L"[MOUSE] Rotate Camera\n");
        helpTextRenderer->text.append(L"[WASD] Move Camera\n");
		helpTextRenderer->text.append(L"[L] Toggle Lighting\n");
		helpTextRenderer->text.append(L"[B] Toggle Blending\n");
		helpTextRenderer->text.append(L"[F9] Screenshot\n");
        helpTextRenderer->text.append(L"[ESC] Quit\n");
    }

	// FPS counter
	fpsTextRenderer->text = std::to_wstring(timer.GetFramesPerSecond()) + L" fps";

    // Check if there is a file that should be loaded delayed
    if (timeUntilLoadFile > 0)
    {
        timeUntilLoadFile -= dt;

        if (timeUntilLoadFile < 0)
        {
            LoadFile();
        }
    }
    else if (Input::GetKeyDown(Keyboard::O))
    {
        // Open file dialog to load another file
        wchar_t filename[MAX_PATH];
        OPENFILENAMEW openFileName;
        ZeroMemory(&openFileName, sizeof(OPENFILENAMEW));
        openFileName.lStructSize = sizeof(OPENFILENAMEW);
        openFileName.hwndOwner = hwnd;
        openFileName.lpstrFilter = L"Ply Files\0*.ply\0\0";
        openFileName.lpstrFile = filename;
        openFileName.lpstrFile[0] = L'\0';
        openFileName.nMaxFile = MAX_PATH;
        openFileName.lpstrTitle = L"Select a file to open!";
        openFileName.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
        openFileName.nFilterIndex = 1;

		// Disable fullscreen in order to avoid issues with selecting the file
		SetFullscreen(false);
        Input::SetMode(Mouse::MODE_ABSOLUTE);

        if (GetOpenFileNameW(&openFileName))
        {
            DelayedLoadFile(filename);
        }

        Input::SetMode(Mouse::MODE_RELATIVE);
    }

    // Save config file and exit on ESC
    if (Input::GetKeyDown(Keyboard::Escape))
    {
        DestroyWindow(hwnd);
    }

    Hierarchy::UpdateAllSceneObjects();
}

void Scene::Draw()
{
	// Set the lighting constant buffer
	lightingConstantBufferData.lightIntensity = settings->lightIntensity;
	lightingConstantBufferData.ambient = settings->ambient;
	lightingConstantBufferData.diffuse = settings->diffuse;
	lightingConstantBufferData.specular = settings->specular;
	lightingConstantBufferData.specularExponent = settings->specularExponent;

	// Update the buffer
	d3d11DevCon->UpdateSubresource(lightingConstantBuffer, 0, NULL, &lightingConstantBufferData, 0, 0);

	// Set the buffer for the pixel shader
	d3d11DevCon->PSSetConstantBuffers(1, 1, &lightingConstantBuffer);

    Hierarchy::DrawAllSceneObjects();
}

void Scene::Release()
{
	SAFE_RELEASE(lightingConstantBuffer);

    Hierarchy::ReleaseAllSceneObjects();
}

void PointCloudEngine::Scene::DelayedLoadFile(std::wstring filepath)
{
    std::wifstream file(filepath);

    // Check if the file exists
    if (file.is_open())
    {
        // Load after some delay
        timeUntilLoadFile = 0.1f;
        settings->plyfile = filepath;

		// Hide the startup text
		startupText->transform->scale = Vector3::Zero;

        // Show huge loading text
        loadingText->transform->scale = Vector3::One;
    }
}

void PointCloudEngine::Scene::LoadFile()
{
    // Release resources before loading
    if (pointCloudRenderer != NULL)
    {
        pointCloud->RemoveComponent(pointCloudRenderer);
        SetWindowTextW(hwnd, L"PointCloudEngine");
    }

    try
    {
        // Try to build the octree from the points (takes a long time)
        pointCloudRenderer = new RENDERER(settings->plyfile);
    }
    catch (std::exception e)
    {
		ERROR_MESSAGE(L"Could not open " + settings->plyfile + L"\nOnly .ply files with x,y,z,nx,ny,nz,red,green,blue vertex format are supported!\nYou can use e.g. MeshLab to generate the required format.");

        // Set the pointer to NULL because the creation of the object failed
        pointCloudRenderer = NULL;
    }

    if (pointCloudRenderer != NULL)
    {
        pointCloud->AddComponent(pointCloudRenderer);
        SetWindowTextW(hwnd, (settings->plyfile + L" - PointCloudEngine").c_str());

        // Set camera position in front of the object
        Vector3 boundingBoxPosition;
        float boundingBoxSize;

        pointCloudRenderer->GetBoundingCubePositionAndSize(boundingBoxPosition, boundingBoxSize);

        camera->SetPosition(settings->scale * (boundingBoxPosition - boundingBoxSize * Vector3::UnitZ));
    }

    // Hide loading text
    loadingText->transform->scale = Vector3::Zero;
    timeSinceLoadFile = 0;

	// Show the fps and help text
	helpText->transform->scale = 0.35f * Vector3::One;

    // Reset point cloud
    pointCloud->transform->position = Vector3::Zero;
    pointCloud->transform->rotation = Quaternion::Identity;

    // Reset camera rotation
    cameraPitch = 0;
    cameraYaw = 0;

    // Reset other properties
	lightingConstantBufferData.useLighting = true;
	lightingConstantBufferData.lightDirection = Vector3(0, -0.5f, 1.0f);
}
