#include "Scene.h"

void Scene::Initialize()
{
    // Create the object for the point cloud
    pointCloud = Hierarchy::Create(L"PointCloud");
	waypointRenderer = new WaypointRenderer();
	pointCloud->AddComponent(waypointRenderer);

	// Create text renderer to display the controls
	helpTextRenderer = new TextRenderer(TextRenderer::GetSpriteFont(L"Consolas"), false);
	helpTextRenderer->enabled = false;
	helpText = Hierarchy::Create(L"Help Text");
	helpText->AddComponent(helpTextRenderer);
	helpText->transform->position = Vector3(-1, 1, 0.5f);
	helpText->transform->scale = 0.35f * Vector3::One;

	// Text for showing properties
	textRenderer = new TextRenderer(TextRenderer::GetSpriteFont(L"Consolas"), false);
	textRenderer->enabled = false;
	text = Hierarchy::Create(L"OctreeRendererText");
	text->AddComponent(textRenderer);
	text->transform->position = Vector3(-1.0f, -0.635f, 0);
	text->transform->scale = 0.35f * Vector3::One;

	// Create fps text in top right corner
	fpsTextRenderer = new TextRenderer(TextRenderer::GetSpriteFont(L"Consolas"), false);
	fpsTextRenderer->enabled = false;
	fpsText = Hierarchy::Create(L"FPS Text");
	fpsText->AddComponent(fpsTextRenderer);
	fpsText->transform->position = Vector3(0.825f, 1, 0.5f);
	fpsText->transform->scale = 0.35f * Vector3::One;

	// Create startup text
	startupTextRenderer = new TextRenderer(TextRenderer::GetSpriteFont(L"Arial"), false);
	startupTextRenderer->text = L"Welcome to PointCloudEngine!\nThis engine renders .pointcloud files.\nYou can convert .ply files with the PlyToPointcloud.exe!\nThis requires .ply files with x,y,z,nx,ny,nz,red,green,blue format.\nYou can change parameters (resolution, ...) in the Settings.txt file.\n\n\nPress [O] to open a .pointcloud file.";
	startupText = Hierarchy::Create(L"Startup Text");
	startupText->AddComponent(startupTextRenderer);
	startupText->transform->scale = Vector3(0.25, 0.35, 1);
	startupText->transform->position = Vector3(-0.95f, 0.4f, 0.5f);

    // Create loading text and hide it
    loadingTextRenderer = new TextRenderer(TextRenderer::GetSpriteFont(L"Arial"), false);
	loadingTextRenderer->enabled = false;
    loadingTextRenderer->text = L"Loading...";
    loadingText = Hierarchy::Create(L"Loading Text");
    loadingText->AddComponent(loadingTextRenderer);
	loadingText->transform->scale = Vector3::One;
    loadingText->transform->position = Vector3(-0.5f, 0.25f, 0.5f);

	hwndSettings = CreateWindowEx(NULL, L"STATIC", L"", WS_CHILD | WS_BORDER | WS_VISIBLE, 0, 0, 0, 0, hwnd, NULL, NULL, NULL);
	hwndDropdown = CreateWindowEx(NULL, L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_CHILD | WS_VISIBLE, 25, 50, 100, 20, hwndSettings, NULL, NULL, NULL);

    // Try to load the last pointcloudFile
    LoadFile(settings->pointcloudFile);
}

void Scene::Update(Timer &timer)
{
	// Toggle help
	if (Input::GetKeyDown(Keyboard::H))
	{
		settings->help = !settings->help;
	}

	// Toggle text visibility
	if (Input::GetKeyDown(Keyboard::T))
	{
		textRenderer->enabled = !textRenderer->enabled;
		helpTextRenderer->enabled = !helpTextRenderer->enabled;
		fpsTextRenderer->enabled = !fpsTextRenderer->enabled;
	}

	// Screenshot on F9
	if (Input::GetKeyDown(Keyboard::F9))
	{
		SaveScreenshotToFile();
	}

	// Toggle view mode depending on the renderer
	if (Input::GetKeyDown(Keyboard::Enter))
	{
		settings->viewMode = (settings->viewMode + 1) % (settings->useOctree ? 3 : 5);
	}

	// Toggle lighting with L
	if (Input::GetKeyDown(Keyboard::L))
	{
		settings->useLighting = !settings->useLighting;
	}

	// Insert a waypoint
	if (Input::GetKeyDown(Keyboard::Insert))
	{
		Vector2 pitchYaw(cameraPitch, cameraYaw);
		waypointRenderer->AddWaypoint(camera->GetPosition(), camera->GetRotationMatrix(), camera->GetForward());
	}

	// Remove a waypoint
	if (Input::GetKeyDown(Keyboard::Delete))
	{
		waypointRenderer->RemoveWaypoint();
	}

	// Camera tracking shot using the waypoints
	if (Input::GetKeyDown(Keyboard::Space))
	{
		// Start preview
		waypointPreviewLocation = 0;
		waypointStartPosition = camera->GetPosition();
	}
	else if (Input::GetKeyUp(Keyboard::Space))
	{
		// End of preview
		camera->SetPosition(waypointStartPosition);
		camera->SetRotationMatrix(Matrix::CreateFromYawPitchRoll(cameraYaw, cameraPitch, 0));
	}
	else if (Input::GetKey(Keyboard::Space))
	{
		// While preview
		Vector3 newCameraPosition = camera->GetPosition();
		Matrix newCameraRotation = camera->GetRotationMatrix();

		waypointRenderer->LerpWaypoints(waypointPreviewLocation, newCameraPosition, newCameraRotation);
		waypointPreviewLocation += settings->waypointPreviewStepSize;

		camera->SetPosition(newCameraPosition);
		camera->SetRotationMatrix(newCameraRotation);
	}
	else
	{
		// Only rotate the camera around the scene while pressing the right mouse button
		if (Input::GetMouseButton(MouseButton::RightButton))
		{
			// Rotate camera with mouse, make sure that this doesn't happen with the accumulated input right after the file loaded
			cameraYaw += Input::mouseDelta.x;
			cameraPitch += Input::mouseDelta.y;
			cameraPitch = cameraPitch > XM_PI / 2.1f ? XM_PI / 2.1f : (cameraPitch < -XM_PI / 2.1f ? -XM_PI / 2.1f : cameraPitch);
			camera->SetRotationMatrix(Matrix::CreateFromYawPitchRoll(cameraYaw, cameraPitch, 0));
		}

		// Move camera with WASD keys
		camera->TranslateRUF(inputSpeed * dt * (Input::GetKey(Keyboard::D) - Input::GetKey(Keyboard::A)), 0, inputSpeed * dt * (Input::GetKey(Keyboard::W) - Input::GetKey(Keyboard::S)));
	}

	// Set the sampling rate (minimal distance between two points) of the loaded point cloud
	if (Input::GetKey(Keyboard::Q))
	{
		if (!settings->useOctree && (settings->viewMode == 1 || settings->viewMode == 3))
		{
			settings->sparseSamplingRate = max(0, settings->sparseSamplingRate - dt * 0.001f * inputSpeed);
		}
		else
		{
			settings->samplingRate = max(0, settings->samplingRate - dt * 0.001f * inputSpeed);
		}
	}
	else if (Input::GetKey(Keyboard::E))
	{
		if (!settings->useOctree && (settings->viewMode == 1 || settings->viewMode == 3))
		{
			settings->sparseSamplingRate += dt * 0.001f * inputSpeed;
		}
		else
		{
			settings->samplingRate += dt * 0.001f * inputSpeed;
		}
	}

	// Toggle blending
	if (Input::GetKeyDown(Keyboard::B))
	{
		settings->useBlending = !settings->useBlending;
	}

	// Change the blend factor
	if (Input::GetKey(Keyboard::V))
	{
		settings->blendFactor = max(0, settings->blendFactor - dt * 0.5f);
	}
	else if (Input::GetKey(Keyboard::N))
	{
		settings->blendFactor += dt * 0.5f;
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

    // Increase input speed when pressing shift and one of the other keys
    if (Input::GetKey(Keyboard::LeftShift))
    {
		if (Input::GetKey(Keyboard::W) || Input::GetKey(Keyboard::A) || Input::GetKey(Keyboard::S) || Input::GetKey(Keyboard::D) || Input::GetKey(Keyboard::Q) || Input::GetKey(Keyboard::E))
		{
			inputSpeed += 20 * dt;
		}
    }
    else
    {
        inputSpeed = 3;
    }

	if (Input::GetMouseButtonDown(MouseButton::RightButton))
	{
		// Hide cursor and focus main window
		SetFocus(hwnd);
		Input::SetMode(Mouse::MODE_RELATIVE);
	}
	else if (Input::GetMouseButtonUp(MouseButton::RightButton))
	{
		// Show cursor
		Input::SetMode(Mouse::MODE_ABSOLUTE);
	}

	// Switch between the two renderers at runtime
	if (Input::GetKeyDown(Keyboard::R))
	{
		settings->useOctree = !settings->useOctree;
		LoadFile(settings->pointcloudFile);
	}

	// FPS counter
	fpsTextRenderer->text = std::to_wstring(timer.GetFramesPerSecond()) + L" fps";

	// Set the other text
	if (pointCloudRenderer != NULL)
	{
		pointCloudRenderer->SetText(text->transform, textRenderer);
		pointCloudRenderer->SetHelpText(helpText->transform, helpTextRenderer);
	}

	// Open file dialog to load another file
    if (Input::GetKeyDown(Keyboard::O))
    {
		OpenFileDialog();
    }

	// FUN ANIMATION FOR TESTING
	MoveWindow(hwndSettings, 20, 20, (1 + 0.2f * sin(clock() / (float)CLOCKS_PER_SEC)) * settings->resolutionX / 4, settings->resolutionY / 2, true);

    // Save config file and exit on ESC
    if (Input::GetKeyDown(Keyboard::Escape))
    {
        DestroyWindow(hwnd);
    }

	Hierarchy::UpdateAllSceneObjects();
}

void Scene::Draw()
{
	Hierarchy::CalculateWorldMatrices();

	if (pointCloudRenderer == NULL)
	{
		startupTextRenderer->Draw();
	}
	else
	{
		Hierarchy::DrawAllSceneObjects();
	}
}

void Scene::Release()
{
	Hierarchy::ReleaseAllSceneObjects();
}

void PointCloudEngine::Scene::OpenFileDialog()
{
	// Show windows explorer open file dialog
	wchar_t filename[MAX_PATH];
	OPENFILENAMEW openFileName;
	ZeroMemory(&openFileName, sizeof(OPENFILENAMEW));
	openFileName.lStructSize = sizeof(OPENFILENAMEW);
	openFileName.hwndOwner = hwnd;
	openFileName.lpstrFilter = L"Pointcloud Files\0*.pointcloud\0\0";
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
		LoadFile(filename);
	}
}

void PointCloudEngine::Scene::LoadFile(std::wstring filepath)
{
	// Check if the file exists
	std::wifstream file(filepath);

	if (!file.is_open())
	{
		// Show startup text
		startupTextRenderer->enabled = true;
		return;
	}

    // Release resources before loading
    if (pointCloudRenderer != NULL)
    {
		pointCloudRenderer->RemoveComponentFromSceneObject();
        SetWindowTextW(hwnd, L"PointCloudEngine");
    }

    try
    {
		// Clear screen and depth and initialize everything so that we can draw (possibly before any update)
		d3d11DevCon->ClearRenderTargetView(renderTargetView, (float*)&settings->backgroundColor);
		d3d11DevCon->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
		d3d11DevCon->OMSetRenderTargets(1, &renderTargetView, depthStencilView);
		Hierarchy::CalculateWorldMatrices();
		camera->PrepareDraw();

		// Show loading screen
		loadingTextRenderer->enabled = true;
		loadingTextRenderer->Draw();
		loadingTextRenderer->enabled = false;

		// Present the result on the screen
		swapChain->Present(0, 0);

		// Set the path for the new file
		settings->pointcloudFile = filepath;

		if (settings->useOctree)
		{
			// Try to build the octree from the points (takes a long time)
			pointCloudRenderer = new OctreeRenderer(settings->pointcloudFile);
		}
		else
		{
			pointCloudRenderer = new GroundTruthRenderer(settings->pointcloudFile);
		}

    }
    catch (std::exception e)
    {
		ERROR_MESSAGE(L"Could not open " + settings->pointcloudFile + L"\nOnly .pointcloud files with x,y,z,nx,ny,nz,red,green,blue vertex format are supported!\nUse e.g. MeshLab and Ply2Pointcloud.exe to convert .ply files to the required format.");

        // Set the pointer to NULL because the creation of the object failed
        pointCloudRenderer = NULL;
    }

    if (pointCloudRenderer != NULL)
    {
        pointCloud->AddComponent(pointCloudRenderer);
        SetWindowTextW(hwnd, ((settings->useOctree ? L"Octree Renderer - " : L"Ground Truth Renderer - ") + settings->pointcloudFile).c_str());

        // Set camera position in front of the object
        Vector3 boundingBoxPosition;
        float boundingBoxSize;

        pointCloudRenderer->GetBoundingCubePositionAndSize(boundingBoxPosition, boundingBoxSize);

        camera->SetPosition(settings->scale * (boundingBoxPosition - boundingBoxSize * Vector3::UnitZ));
    }

	// Show the other text
	startupTextRenderer->enabled = false;
	helpTextRenderer->enabled = true;
	fpsTextRenderer->enabled = true;
	textRenderer->enabled = true;

    // Reset point cloud
    pointCloud->transform->position = Vector3::Zero;
    pointCloud->transform->rotation = Quaternion::Identity;

    // Reset camera rotation
	camera->SetRotationMatrix(Matrix::CreateFromYawPitchRoll(0, 0, 0));
    cameraPitch = 0;
    cameraYaw = 0;
}
