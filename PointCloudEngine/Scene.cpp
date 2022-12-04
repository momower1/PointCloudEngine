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
		meshRenderer->UpdatePreviousMatrices();
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
	ViewMode startViewMode = settings->viewMode;
	ShadingMode startShadingMode = settings->shadingMode;
	Vector3 startPosition = camera->GetPosition();
	Matrix startRotation = camera->GetRotationMatrix();

	if (waypointRenderer != NULL)
	{
		float start = settings->waypointMin * waypointRenderer->GetWaypointSize();
		float end = settings->waypointMax * waypointRenderer->GetWaypointSize();

		UINT counter = 0;
		float waypointLocation = start;
		Vector3 newCameraPosition = camera->GetPosition();
		Matrix newCameraRotation = camera->GetRotationMatrix();

		while ((waypointLocation < end) && waypointRenderer->LerpWaypoints(waypointLocation, newCameraPosition, newCameraRotation))
		{
			camera->SetPosition(newCameraPosition);
			camera->SetRotationMatrix(newCameraRotation);

			DrawAndSaveDatasetEntry(counter);
			counter++;

			waypointLocation += settings->waypointStepSize;
		}
	}

	// Reset properties
	camera->SetPosition(startPosition);
	camera->SetRotationMatrix(startRotation);
	settings->viewMode = startViewMode;
	settings->shadingMode = startShadingMode;
}

void PointCloudEngine::Scene::GenerateSphereDataset()
{
	ViewMode startViewMode = settings->viewMode;
	ShadingMode startShadingMode = settings->shadingMode;
	Vector3 startPosition = camera->GetPosition();
	Matrix startRotation = camera->GetRotationMatrix();

	float boundingCubeSize;
	Vector3 boundingCubePosition;
	pointCloudRenderer->GetBoundingCubePositionAndSize(boundingCubePosition, boundingCubeSize);

	Vector3 center = boundingCubePosition * pointCloud->transform->scale;
	float r = Vector3::Distance(camera->GetPosition(), center);

	UINT counter = 0;
	float h = settings->sphereStepSize;

	for (float theta = settings->sphereMinTheta + h / 2; theta < settings->sphereMaxTheta; theta += h / 2)
	{
		for (float phi = settings->sphereMinPhi + h; phi < settings->sphereMaxPhi; phi += h)
		{
			// Rotate around and look at the center
			Vector3 newPosition = center + r * Vector3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
			camera->SetPosition(newPosition);
			camera->LookAt(center);

			DrawAndSaveDatasetEntry(counter);
			counter++;
		}
	}

	// Reset properties
	camera->SetPosition(startPosition);
	camera->SetRotationMatrix(startRotation);
	settings->viewMode = startViewMode;
	settings->shadingMode = startShadingMode;
}

void PointCloudEngine::Scene::DrawAndSaveDatasetEntry(UINT index)
{
	std::wstring datasetDirectory = L"D:/Downloads/PointCloudEngineDataset/";
	std::wstring datasetFilenames = L"";

	// Go over all the render modes
	for (auto it = datasetRenderModes.begin(); it != datasetRenderModes.end(); it++)
	{
		// Clear the render target and depth/stencil view
		d3d11DevCon->ClearRenderTargetView(renderTargetView, (float*)&settings->backgroundColor);
		d3d11DevCon->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
		d3d11DevCon->OMSetRenderTargets(1, &renderTargetView, depthStencilView);
		d3d11DevCon->OMSetDepthStencilState(depthStencilState, 0);

		camera->PrepareDraw();

		settings->viewMode = it->viewMode;
		settings->shadingMode = it->shadingMode;

		if (it->viewMode == ViewMode::Mesh)
		{
			meshRenderer->Draw();
		}
		else
		{
			pointCloudRenderer->GetComponent()->Draw();
		}

		D3D11_TEXTURE2D_DESC readableTextureDesc;
		backBufferTexture->GetDesc(&readableTextureDesc);

		if (readableTextureDesc.Format != DXGI_FORMAT_R16G16B16A16_FLOAT)
		{
			ERROR_MESSAGE(L"Cannot save dataset texture since it is not in DXGI_FORMAT_R16G16B16A16_FLOAT format!");
		}

		readableTextureDesc.Usage = D3D11_USAGE_STAGING;
		readableTextureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		readableTextureDesc.BindFlags = 0;
		readableTextureDesc.MiscFlags = 0;

		ID3D11Texture2D* readableTexture = NULL;

		hr = d3d11Device->CreateTexture2D(&readableTextureDesc, NULL, &readableTexture);
		ERROR_MESSAGE_ON_HR(hr, NAMEOF(d3d11Device->CreateTexture2D) + L" failed!");

		d3d11DevCon->CopyResource(readableTexture, backBufferTexture);

		D3D11_MAPPED_SUBRESOURCE mappedSubresource;
		ZeroMemory(&mappedSubresource, sizeof(mappedSubresource));
		
		hr = d3d11DevCon->Map(readableTexture, 0, D3D11_MAP_READ, 0, &mappedSubresource);
		ERROR_MESSAGE_ON_HR(hr, NAMEOF(d3d11DevCon->Map) + L" failed!");

		// Create a custom binary file that stores the raw bytes of the texture
		std::wstring datasetFilename = it->name + std::to_wstring(index) + L".texture";
		std::ofstream file(datasetDirectory + datasetFilename, std::ios::out | std::ios::binary);

		// Write a header to the file
		int width = readableTextureDesc.Width;
		int height = readableTextureDesc.Height;
		int channels = 4;
		int elementSizeInBytes = 2;

		file.write((char*)&width, sizeof(int));
		file.write((char*)&height, sizeof(int));
		file.write((char*)&channels, sizeof(int));
		file.write((char*)&elementSizeInBytes, sizeof(int));

		// Need to skip padding that the texture might have in between rows and depth
		size_t rowPitchWithoutPadding = (size_t)width * (size_t)channels * (size_t)elementSizeInBytes;

		for (size_t rowIndex = 0; rowIndex < height; rowIndex++)
		{
			file.write((char*)mappedSubresource.pData + rowIndex * mappedSubresource.RowPitch, rowPitchWithoutPadding);
		}

		file.flush();
		file.close();

		d3d11DevCon->Unmap(readableTexture, 0);

		SAFE_RELEASE(readableTexture);

		datasetFilenames += datasetFilename;

		if ((it + 1) != datasetRenderModes.end())
		{
			datasetFilenames += L", ";
		}

		//SaveDDSTextureToFile(d3d11DevCon, backBufferTexture, (L"D:/Downloads/PointCloudEngineDataset/" + it->name + L".dds").c_str());

		// Need to do everything before presenting the texture to the screen to preserve alpha channel
		swapChain->Present(0, 0);
	}

	// Explicitly update the previous frame matrices for optical flow computation
	meshRenderer->UpdatePreviousMatrices();

	// Pack all the files into a ZIP archive and delete the old files
	std::wstring command = L"powershell ";
	command += L"cd " + datasetDirectory + L"; ";

	command += L"Compress-Archive ";
	command += L"-Path " + datasetFilenames + L" ";
	command += L"-DestinationPath " + std::to_wstring(index) + L".zip ";
	command += L"-Update ";
	command += L"-CompressionLevel Optimal; ";

	command += L"Remove-Item ";
	command += L"-Path " + datasetFilenames;
	_wsystem(command.c_str());

	std::cout << std::string(command.begin(), command.end()) << std::endl;

	std::cout << "Saved dataset entry " << std::to_string(index) << std::endl;
}
