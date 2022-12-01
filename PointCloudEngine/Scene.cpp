#include "Scene.h"

void Scene::Initialize()
{
    // Create the object for the point cloud
    pointCloud = Hierarchy::Create(L"PointCloud");
	waypointRenderer = new WaypointRenderer();
	pointCloud->AddComponent(waypointRenderer);
	waypointRenderer->enabled = false;

	// Create startup text
	startupTextRenderer = new TextRenderer(TextRenderer::GetSpriteFont(L"Arial"), false);
	startupTextRenderer->color = Vector4(1, 1, 1, 1) - settings->backgroundColor;
	startupTextRenderer->text = L"Welcome to PointCloudEngine!\nThis engine renders .pointcloud files.\nYou can convert .ply files with the PlyToPointcloud.exe!\nThis requires .ply files with x,y,z,nx,ny,nz,red,green,blue format.\nYou can change parameters (resolution, ...) in the Settings.txt file.\n\n\nPress File->Open to open a .pointcloud file.";
	startupText = Hierarchy::Create(L"Startup Text");
	startupText->AddComponent(startupTextRenderer);
	startupText->transform->scale = Vector3(0.25, 0.35, 1);
	startupText->transform->position = Vector3(-0.95f, 0.4f, 0.5f);

    // Create loading text and hide it
    loadingTextRenderer = new TextRenderer(TextRenderer::GetSpriteFont(L"Arial"), false);
	loadingTextRenderer->color = Vector4(1, 1, 1, 1) - settings->backgroundColor;
	loadingTextRenderer->enabled = false;
    loadingTextRenderer->text = L"Loading...";
    loadingText = Hierarchy::Create(L"Loading Text");
    loadingText->AddComponent(loadingTextRenderer);
	loadingText->transform->scale = Vector3::One;
    loadingText->transform->position = Vector3(-0.5f, 0.25f, 0.5f);

    // Try to load the last pointcloudFile
    LoadFile(settings->pointcloudFile);

	// Load the last mesh file as well
	if (settings->meshFile.length() > 0)
	{
		settings->loadMeshFile = true;
	}
}

void Scene::Update(Timer &timer)
{
	// Possibly load or reload the mesh
	if ((pointCloud != NULL) && settings->loadMeshFile)
	{
		if (meshRenderer != NULL)
		{
			pointCloud->RemoveComponent(meshRenderer);
		}

		OBJContainer container;
		
		if (OBJFile::LoadOBJFile(settings->meshFile, &container))
		{
			meshRenderer = new MeshRenderer(container);
			pointCloud->AddComponent(meshRenderer);
		}

		settings->loadMeshFile = false;
	}

	// Toggle between rendering the mesh or the point cloud
	if (meshRenderer != NULL)
	{
		meshRenderer->enabled = (settings->viewMode == ViewMode::Mesh);
	}

	if (pointCloudRenderer != NULL)
	{
		pointCloudRenderer->GetComponent()->enabled = (settings->viewMode != ViewMode::Mesh);
	}

	// Camera tracking shot using the waypoints
	if (waypointPreview)
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

    // Scale the point cloud by the value saved in the config file
	// Use a negative scale for the X-component to convert the point cloud and mesh to a left handed coordinate system
	if (pointCloud != NULL)
	{
		pointCloud->transform->scale = Vector3(settings->useOctree ? settings->scale : -settings->scale, settings->scale, settings->scale);
	}

    // Increase input speed when pressing shift and one of the other keys
    if (Input::GetKey(Keyboard::LeftShift))
    {
		if (Input::GetKey(Keyboard::W) || Input::GetKey(Keyboard::A) || Input::GetKey(Keyboard::S) || Input::GetKey(Keyboard::D))
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
		SetFocus(hwndScene);
		Input::SetMode(Mouse::MODE_RELATIVE);
	}
	else if (Input::GetMouseButtonUp(MouseButton::RightButton))
	{
		// Show cursor
		Input::SetMode(Mouse::MODE_ABSOLUTE);
	}

	// Show fps
	GUI::fps = timer.GetFramesPerSecond();

    // Save config file and exit on ESC
    if (Input::GetKeyDown(Keyboard::Escape))
    {
        DestroyWindow(hwndScene);
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

void PointCloudEngine::Scene::OpenPlyOrPointcloudFile()
{
	// Disable fullscreen in order to avoid issues with selecting the file
	SetFullscreen(false);
	Input::SetMode(Mouse::MODE_ABSOLUTE);

	std::wstring filename;

	if (Utils::OpenFileDialog(L"Pointcloud Files\0*.pointcloud\0Ply Files\0*.ply\0\0", filename))
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

	std::wstring fileExtension = filepath.substr(filepath.find_last_of(L'.'));

	// Possibly need to convert the .ply file into a .pointcloud file (execute PlyToPointcloud.exe and wait for it to finish)
	if (fileExtension.compare(L".ply") == 0)
	{
		std::wstring plyToPointcloudPath = executableDirectory + L"\\PlyToPointcloud.exe";

		SHELLEXECUTEINFO shellExecuteInfo;
		ZeroMemory(&shellExecuteInfo, sizeof(shellExecuteInfo));
		shellExecuteInfo.cbSize = sizeof(shellExecuteInfo);
		shellExecuteInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
		shellExecuteInfo.hwnd = NULL;
		shellExecuteInfo.lpVerb = L"open";
		shellExecuteInfo.lpFile = plyToPointcloudPath.c_str();
		shellExecuteInfo.lpParameters = filepath.c_str();
		shellExecuteInfo.lpDirectory = NULL;
		shellExecuteInfo.nShow = SW_SHOW;
		shellExecuteInfo.hInstApp = NULL;

		ShellExecuteEx(&shellExecuteInfo);
		WaitForSingleObject(shellExecuteInfo.hProcess, INFINITE);
		CloseHandle(shellExecuteInfo.hProcess);

		filepath = filepath.substr(0, filepath.find_last_of(L'.')) + L".pointcloud";
	}

    // Release resources before loading
    if (pointCloudRenderer != NULL)
    {
		pointCloudRenderer->RemoveComponentFromSceneObject();
        SetWindowTextW(hwndEngine, L"PointCloudEngine");

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
			pointCloudRenderer = groundTruthRenderer;
		}

    }
    catch (std::exception e)
    {
		ERROR_MESSAGE(L"Could not open " + settings->pointcloudFile + L"\nOnly .pointcloud files with x,y,z,nx,ny,nz,red,green,blue vertex format are supported!\nUse e.g. MeshLab and PlyToPointcloud.exe to convert .ply files to the required format.");

        // Set the pointer to NULL because the creation of the object failed
        pointCloudRenderer = NULL;
    }

    if (pointCloudRenderer != NULL)
    {
        pointCloud->AddComponent(pointCloudRenderer);
        SetWindowTextW(hwndEngine, ((settings->useOctree ? L"Octree Renderer - " : L"Ground Truth Renderer - ") + settings->pointcloudFile).c_str());

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

void PointCloudEngine::Scene::AddWaypoint()
{
	if (waypointRenderer != NULL)
	{
		waypointRenderer->enabled = true;
		waypointRenderer->AddWaypoint(camera->GetPosition(), camera->GetRotationMatrix(), camera->GetForward());
	}
}

void PointCloudEngine::Scene::RemoveWaypoint()
{
	if (waypointRenderer != NULL)
	{
		waypointRenderer->enabled = true;
		waypointRenderer->RemoveWaypoint();
	}
}

void PointCloudEngine::Scene::ToggleWaypoints()
{
	if (waypointRenderer != NULL)
	{
		waypointRenderer->enabled = !waypointRenderer->enabled;
	}
}

void PointCloudEngine::Scene::PreviewWaypoints()
{
	if (waypointRenderer != NULL)
	{
		waypointRenderer->enabled = true;
		waypointPreview = !waypointPreview;

		// Camera tracking shot using the waypoints
		if (waypointPreview)
		{
			// Start preview
			waypointPreviewLocation = 0;
			waypointStartPosition = camera->GetPosition();
			waypointStartRotation = camera->GetRotationMatrix();
		}
		else
		{
			// End of preview
			camera->SetPosition(waypointStartPosition);
			camera->SetRotationMatrix(waypointStartRotation);
		}
	}
}

void PointCloudEngine::Scene::GenerateWaypointDataset()
{
	std::cout << "TODO" << std::endl;
}

void PointCloudEngine::Scene::GenerateSphereDataset()
{
	std::cout << "TODO" << std::endl;
}
