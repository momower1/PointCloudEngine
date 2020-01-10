#include "Scene.h"

void Scene::Initialize()
{
    // Create the object for the point cloud
    pointCloud = Hierarchy::Create(L"PointCloud");
	waypointRenderer = new WaypointRenderer();
	pointCloud->AddComponent(waypointRenderer);
	GUI::waypointRenderer = waypointRenderer;

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

	// Screenshot on F9
	if (Input::GetKeyDown(Keyboard::F9))
	{
		SaveScreenshotToFile();
	}

	// Toggle lighting with L
	if (Input::GetKeyDown(Keyboard::L))
	{
		settings->useLighting = !settings->useLighting;
	}

	// Insert a waypoint
	if (Input::GetKeyDown(Keyboard::Insert))
	{
		waypointRenderer->AddWaypoint(camera->GetPosition(), camera->GetRotationMatrix(), camera->GetForward());
	}

	// Remove a waypoint
	if (Input::GetKeyDown(Keyboard::Delete))
	{
		waypointRenderer->RemoveWaypoint();
	}

	// Camera tracking shot using the waypoints
	if (GUI::waypointPreview)
	{
		// While preview
		Vector3 newCameraPosition = camera->GetPosition();
		Matrix newCameraRotation = camera->GetRotationMatrix();

		waypointRenderer->LerpWaypoints(GUI::waypointPreviewLocation, newCameraPosition, newCameraRotation);
		GUI::waypointPreviewLocation += settings->waypointPreviewStepSize;

		camera->SetPosition(newCameraPosition);
		camera->SetRotationMatrix(newCameraRotation);
	}
	else
	{
		// Only rotate the camera around the scene while pressing the right mouse button
		if (Input::GetMouseButton(MouseButton::RightButton))
		{
			Vector3 ypr = camera->GetYawPitchRoll();

			// Rotate camera with mouse using yaw pitch roll representation
			ypr.x += Input::mouseDelta.x;
			ypr.y += Input::mouseDelta.y;
			ypr.y = ypr.y > XM_PI / 2.1f ? XM_PI / 2.1f : (ypr.y < -XM_PI / 2.1f ? -XM_PI / 2.1f : ypr.y);

			camera->SetRotationMatrix(Matrix::CreateFromYawPitchRoll(ypr.x, ypr.y, 0));
		}

		// Move camera with WASD keys
		camera->TranslateRUF(inputSpeed * dt * (Input::GetKey(Keyboard::D) - Input::GetKey(Keyboard::A)), 0, inputSpeed * dt * (Input::GetKey(Keyboard::W) - Input::GetKey(Keyboard::S)));
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
    settings->scale = max(0.01f, settings->scale + Input::mouseScrollDelta);
    pointCloud->transform->scale = settings->scale * Vector3::One;

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

	// Open file dialog to load another file
    if (Input::GetKeyDown(Keyboard::O))
    {
		OpenFileDialog();
    }

	// Show fps
	GUI::fps = timer.GetFramesPerSecond();

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
	GUI::Release();
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

		// Hide GUI
		GUI::SetVisible(false);
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
			GroundTruthRenderer* groundTruthRenderer = new GroundTruthRenderer(settings->pointcloudFile);
			GUI::groundTruthRenderer = groundTruthRenderer;
			pointCloudRenderer = groundTruthRenderer;
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

		// Show the GUI
		GUI::Initialize();
		GUI::SetVisible(true);
    }

	// Show the other text
	startupTextRenderer->enabled = false;

    // Reset point cloud
    pointCloud->transform->position = Vector3::Zero;
    pointCloud->transform->rotation = Quaternion::Identity;

    // Reset camera rotation
	camera->SetRotationMatrix(Matrix::CreateFromYawPitchRoll(0, 0, 0));
}
