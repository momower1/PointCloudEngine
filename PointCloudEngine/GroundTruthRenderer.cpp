#include "GroundTruthRenderer.h"

GroundTruthRenderer::GroundTruthRenderer(const std::wstring &pointcloudFile)
{
    // Try to load the file
    if (!LoadPointcloudFile(vertices, boundingCubePosition, boundingCubeSize, pointcloudFile))
    {
        throw std::exception("Could not load .pointcloud file!");
    }

    // Set the default values
    constantBufferData.fovAngleY = settings->fovAngleY;
	constantBufferData.drawNormals = false;
}

void GroundTruthRenderer::Initialize()
{
    // Create a vertex buffer description
    D3D11_BUFFER_DESC vertexBufferDesc;
    ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));
    vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    vertexBufferDesc.ByteWidth = sizeof(Vertex) * vertices.size();
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexBufferDesc.CPUAccessFlags = 0;
    vertexBufferDesc.MiscFlags = 0;

    // Fill a D3D11_SUBRESOURCE_DATA struct with the data we want in the buffer
    D3D11_SUBRESOURCE_DATA vertexBufferData;
    ZeroMemory(&vertexBufferData, sizeof(vertexBufferData));
    vertexBufferData.pSysMem = &vertices[0];

    // Create the buffer
    hr = d3d11Device->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &vertexBuffer);
	ERROR_MESSAGE_ON_FAIL(hr, NAMEOF(d3d11Device->CreateBuffer) + L" failed for the " + NAMEOF(vertexBuffer));

    // Create the constant buffer for WVP
    D3D11_BUFFER_DESC constantBufferDesc;
    ZeroMemory(&constantBufferDesc, sizeof(constantBufferDesc));
	constantBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	constantBufferDesc.ByteWidth = sizeof(GroundTruthRendererConstantBuffer);
	constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	constantBufferDesc.CPUAccessFlags = 0;
	constantBufferDesc.MiscFlags = 0;

    hr = d3d11Device->CreateBuffer(&constantBufferDesc, NULL, &constantBuffer);
	ERROR_MESSAGE_ON_FAIL(hr, NAMEOF(d3d11Device->CreateBuffer) + L" failed for the " + NAMEOF(constantBuffer));
}

void GroundTruthRenderer::Update()
{
	// Select density of the point cloud with arrow keys
	if (Input::GetKey(Keyboard::Right))
	{
		settings->density = min(1.0f, settings->density + 0.15f * dt);
	}
	else if (Input::GetKey(Keyboard::Left))
	{
		settings->density = max(0, settings->density - 0.15f * dt);
	}

	// Save HDF5 file
	if (Input::GetKeyDown(Keyboard::F7) || Input::GetKeyDown(Keyboard::F8))
	{
		// Create a directory for the HDF5 files
		CreateDirectory((executableDirectory + L"/HDF5").c_str(), NULL);

		// Create and save the file
		HDF5File hdf5file(executableDirectory + L"/HDF5/" + std::to_wstring(time(0)) + L".hdf5");

		// Add attributes storing the settings
		hdf5file.AddStringAttribute(NAMEOF(settings->pointcloudFile), settings->pointcloudFile);
		hdf5file.AddStringAttribute(NAMEOF(settings->samplingRate), std::to_wstring(settings->samplingRate));
		hdf5file.AddStringAttribute(NAMEOF(settings->scale), std::to_wstring(settings->scale));
		hdf5file.AddStringAttribute(NAMEOF(settings->useLighting), std::to_wstring(settings->useLighting));
		hdf5file.AddStringAttribute(NAMEOF(settings->useHeadlight), std::to_wstring(settings->useHeadlight));
		hdf5file.AddStringAttribute(NAMEOF(settings->lightDirection), settings->ToString(settings->lightDirection));
		hdf5file.AddStringAttribute(NAMEOF(settings->lightIntensity), std::to_wstring(settings->lightIntensity));
		hdf5file.AddStringAttribute(NAMEOF(settings->ambient), std::to_wstring(settings->ambient));
		hdf5file.AddStringAttribute(NAMEOF(settings->diffuse), std::to_wstring(settings->diffuse));
		hdf5file.AddStringAttribute(NAMEOF(settings->specular), std::to_wstring(settings->specular));
		hdf5file.AddStringAttribute(NAMEOF(settings->specularExponent), std::to_wstring(settings->specularExponent));
		hdf5file.AddStringAttribute(NAMEOF(settings->blendFactor), std::to_wstring(settings->blendFactor));
		hdf5file.AddStringAttribute(NAMEOF(settings->density), std::to_wstring(settings->density));
		hdf5file.AddStringAttribute(NAMEOF(settings->sparseSamplingRate), std::to_wstring(settings->sparseSamplingRate));
		hdf5file.AddStringAttribute(NAMEOF(settings->waypointStepSize), std::to_wstring(settings->waypointStepSize));
		hdf5file.AddStringAttribute(NAMEOF(settings->sphereStepSize), std::to_wstring(settings->sphereStepSize));
		hdf5file.AddStringAttribute(NAMEOF(settings->sphereMinTheta), std::to_wstring(settings->sphereMinTheta));
		hdf5file.AddStringAttribute(NAMEOF(settings->sphereMaxTheta), std::to_wstring(settings->sphereMaxTheta));
		hdf5file.AddStringAttribute(NAMEOF(settings->sphereMinPhi), std::to_wstring(settings->sphereMinPhi));
		hdf5file.AddStringAttribute(NAMEOF(settings->sphereMaxPhi), std::to_wstring(settings->sphereMaxPhi));

		int startViewMode = settings->viewMode;
		Vector3 startPosition = camera->GetPosition();
		Matrix startRotation = camera->GetRotationMatrix();

		if (Input::GetKeyDown(Keyboard::F7))
		{
			GenerateWaypointDataset(hdf5file);
		}
		else
		{
			GenerateSphereDataset(hdf5file);
		}

		// Reset properties
		camera->SetPosition(startPosition);
		camera->SetRotationMatrix(startRotation);
		settings->viewMode = startViewMode;
	}
}

void GroundTruthRenderer::Draw()
{
	if (settings->viewMode < 2)
	{
		// Set the splat shaders
		d3d11DevCon->VSSetShader(splatShader->vertexShader, 0, 0);
		d3d11DevCon->GSSetShader(splatShader->geometryShader, 0, 0);
		d3d11DevCon->PSSetShader(splatShader->pixelShader, 0, 0);
	}
	else
	{
		// Set the point shaders
		d3d11DevCon->VSSetShader(pointShader->vertexShader, 0, 0);
		d3d11DevCon->GSSetShader(pointShader->geometryShader, 0, 0);
		d3d11DevCon->PSSetShader(pointShader->pixelShader, 0, 0);
	}

    // Set the Input (Vertex) Layout
    d3d11DevCon->IASetInputLayout(splatShader->inputLayout);

    // Bind the vertex buffer and index buffer to the input assembler (IA)
    UINT offset = 0;
    UINT stride = sizeof(Vertex);
    d3d11DevCon->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);

    // Set primitive topology
    d3d11DevCon->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

    // Set shader constant buffer variables
    constantBufferData.World = sceneObject->transform->worldMatrix.Transpose();
	constantBufferData.View = camera->GetViewMatrix().Transpose();
	constantBufferData.Projection = camera->GetProjectionMatrix().Transpose();
    constantBufferData.WorldInverseTranspose = constantBufferData.World.Invert().Transpose();
	constantBufferData.WorldViewProjectionInverse = (sceneObject->transform->worldMatrix * camera->GetViewMatrix() * camera->GetProjectionMatrix()).Invert().Transpose();
    constantBufferData.cameraPosition = camera->GetPosition();
	constantBufferData.blendFactor = settings->blendFactor;
	constantBufferData.useBlending = false;

	// The amount of points that will be drawn
	UINT vertexCount = vertices.size();

	// Set different sampling rates based on the view mode
	if (settings->viewMode == 0 || settings->viewMode == 2)
	{
		constantBufferData.samplingRate = settings->samplingRate;
	}
	else
	{
		constantBufferData.samplingRate = settings->sparseSamplingRate;

		// Only draw a portion of the point cloud to simulate the selected density
		// This requires the vertex indices to be distributed randomly (pointcloud files provide this feature)
		vertexCount *= settings->density;
	}

    // Update effect file buffer, set shader buffer to our created buffer
    d3d11DevCon->UpdateSubresource(constantBuffer, 0, NULL, &constantBufferData, 0, 0);
	d3d11DevCon->VSSetConstantBuffers(0, 1, &constantBuffer);
	d3d11DevCon->GSSetConstantBuffers(0, 1, &constantBuffer);
	d3d11DevCon->PSSetConstantBuffers(0, 1, &constantBuffer);

	if ((settings->viewMode < 2) && settings->useBlending)
	{
		DrawBlended(vertexCount, constantBuffer, &constantBufferData, constantBufferData.useBlending);
	}
	else
	{
		d3d11DevCon->Draw(vertexCount, 0);
	}

	// Evaluate neural network and present the result to the screen
	if (settings->viewMode == 4)
	{
		DrawNeuralNetwork();
	}
}

void GroundTruthRenderer::Release()
{
    SAFE_RELEASE(vertexBuffer);
    SAFE_RELEASE(constantBuffer);
	SafeDelete(torchDevice);
}

void PointCloudEngine::GroundTruthRenderer::GetBoundingCubePositionAndSize(Vector3 &outPosition, float &outSize)
{
	outPosition = boundingCubePosition;
	outSize = boundingCubeSize;
}

void PointCloudEngine::GroundTruthRenderer::SetHelpText(Transform* helpTextTransform, TextRenderer* helpTextRenderer)
{
	helpTextTransform->position = Vector3(-1, 1, 0.5f);
	helpTextRenderer->text = L"[H] Toggle help\n";

	if (settings->help)
	{
		helpTextRenderer->text.append(L"[O] Open .pointcloud file\n");
		helpTextRenderer->text.append(L"[T] Toggle text visibility\n");
		helpTextRenderer->text.append(L"[R] Switch to octree renderer\n");
		helpTextRenderer->text.append(L"[E/Q] Increase/decrease sampling rate\n");
		helpTextRenderer->text.append(L"[N/V] Increase/decrease blend factor\n");
		helpTextRenderer->text.append(L"[SHIFT] Increase WASD and Q/E input speed\n");
		helpTextRenderer->text.append(L"[RIGHT/LEFT] Increase/decrease point cloud density\n");
		helpTextRenderer->text.append(L"[ENTER] Switch view mode\n");
		helpTextRenderer->text.append(L"[INSERT] Add camera waypoint\n");
		helpTextRenderer->text.append(L"[DELETE] Remove camera waypoint\n");
		helpTextRenderer->text.append(L"[SPACE] Preview camera trackshot\n");
		helpTextRenderer->text.append(L"[F1-F6] Select camera position\n");
		helpTextRenderer->text.append(L"[F7] Generate Waypoint HDF5 Dataset\n");
		helpTextRenderer->text.append(L"[F8] Generate Sphere HDF5 Dataset\n");
		helpTextRenderer->text.append(L"[MOUSE WHEEL] Scale\n");
		helpTextRenderer->text.append(L"[MOUSE] Rotate Camera\n");
		helpTextRenderer->text.append(L"[WASD] Move Camera\n");
		helpTextRenderer->text.append(L"[L] Toggle Lighting\n");
		helpTextRenderer->text.append(L"[B] Toggle Blending\n");
		helpTextRenderer->text.append(L"[F9] Screenshot\n");
		helpTextRenderer->text.append(L"[ESC] Quit\n");
	}
}

void PointCloudEngine::GroundTruthRenderer::SetText(Transform* textTransform, TextRenderer* textRenderer)
{
	if (settings->viewMode == 4)
	{
		textTransform->position = Vector3(-1.0f, -0.735f, 0);
		textRenderer->text = std::wstring(L"View Mode: Neural Network\n");
	}
	else if (settings->viewMode % 2 == 0)
	{
		textTransform->position = Vector3(-1.0f, -0.735f, 0);

		if (settings->viewMode == 0)
		{
			textRenderer->text = std::wstring(L"View Mode: Splats\n");
		}
		else
		{
			textRenderer->text = std::wstring(L"View Mode: Points\n");
		}

		textRenderer->text.append(L"Sampling Rate: " + std::to_wstring(settings->samplingRate) + L"\n");
		textRenderer->text.append(L"Blend Factor: " + std::to_wstring(settings->blendFactor) + L"\n");
		textRenderer->text.append(L"Blending " + std::wstring(settings->useBlending ? L"On, " : L"Off, "));
		textRenderer->text.append(L"Lighting " + std::wstring(settings->useLighting ? L"On\n" : L"Off\n"));
		textRenderer->text.append(L"Vertex Count: " + std::to_wstring(vertices.size()) + L"\n");
	}
	else
	{
		textTransform->position = Vector3(-1.0f, -0.685f, 0);

		if (settings->viewMode == 1)
		{
			textRenderer->text = std::wstring(L"View Mode: Sparse Splats\n");
		}
		else
		{
			textRenderer->text = std::wstring(L"View Mode: Sparse Points\n");
		}

		textRenderer->text.append(L"Sampling Rate: " + std::to_wstring(settings->sparseSamplingRate) + L"\n");
		textRenderer->text.append(L"Blend Factor: " + std::to_wstring(settings->blendFactor) + L"\n");
		textRenderer->text.append(L"Point Density: " + std::to_wstring(settings->density * 100) + L"%\n");
		textRenderer->text.append(L"Blending " + std::wstring(settings->useBlending ? L"On, " : L"Off, "));
		textRenderer->text.append(L"Lighting " + std::wstring(settings->useLighting ? L"On\n" : L"Off\n"));
		textRenderer->text.append(L"Vertex Count: " + std::to_wstring((UINT)(vertices.size() * settings->density)) + L"\n");
	}
}

void PointCloudEngine::GroundTruthRenderer::RemoveComponentFromSceneObject()
{
	sceneObject->RemoveComponent(this);
}

void PointCloudEngine::GroundTruthRenderer::DrawNeuralNetwork()
{
	if (torchDevice == NULL)
	{
		// Load the neural network from file
		if (torch::cuda::is_available())
		{
			torchDevice = new torch::Device(at::kCUDA);
		}
		else
		{
			torchDevice = new torch::Device(at::kCPU);
		}

		std::wstring modelFilename = executableDirectory + L"\\NeuralNetwork.pt";

		try
		{
			model = torch::jit::load(std::string(modelFilename.begin(), modelFilename.end()), *torchDevice);
		}
		catch (const std::exception & e)
		{
			ERROR_MESSAGE(L"Could not load Pytorch Jit Neural Network from file " + modelFilename);
		}
	}
	else
	{
		// Get the data from the backbuffer
		ID3D11Texture2D* tensorTexture = NULL;
		D3D11_TEXTURE2D_DESC tensorTextureDesc;

		// Copy it to a cpu readable and writeable texture
		backBufferTexture->GetDesc(&tensorTextureDesc);
		tensorTextureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
		tensorTextureDesc.Usage = D3D11_USAGE_STAGING;
		tensorTextureDesc.BindFlags = 0;

		hr = d3d11Device->CreateTexture2D(&tensorTextureDesc, NULL, &tensorTexture);
		ERROR_MESSAGE_ON_FAIL(hr, NAMEOF(d3d11Device->CreateTexture2D) + L" failed!");

		d3d11DevCon->CopyResource(tensorTexture, backBufferTexture);

		// Read the raw texture data
		D3D11_MAPPED_SUBRESOURCE subresource;
		d3d11DevCon->Map(tensorTexture, 0, D3D11_MAP_READ, 0, &subresource);

		// Convert the texture data to a tensor for the neural network
		torch::Tensor input = torch::zeros({ 1, 2, settings->resolutionX, settings->resolutionY }, torch::dtype(torch::kHalf));

		// Copy the data from the texture to the tensor
		memcpy(input.data_ptr(), subresource.pData, 2 * 2 * settings->resolutionX * settings->resolutionY);

		// Unmap the texture
		d3d11DevCon->Unmap(tensorTexture, 0);

		// Convert to 32bit float and push to GPU if possible
		input = input.to(torch::dtype(torch::kFloat32));

		if (torch::cuda::is_available())
		{
			input = input.cuda();
		}

		std::vector<torch::jit::IValue> inputs;
		inputs.push_back(input);

		// Evaluate the model
		torch::Tensor output = model.forward(inputs).toTensor();

		// Put the tensor back to 16bit float and on the cpu in order to read it
		output = output.to(torch::dtype(torch::kHalf));
		output = output.cpu();

		// Copy the output data to the texture
		d3d11DevCon->Map(tensorTexture, 0, D3D11_MAP_WRITE, 0, &subresource);

		// Copy the data from the texture to the tensor
		memcpy(subresource.pData, output.data_ptr(), 2 * 2 * settings->resolutionX * settings->resolutionY);

		// Unmap the texture
		d3d11DevCon->Unmap(tensorTexture, 0);

		//SaveWICTextureToFile(d3d11DevCon, tensorTexture, GUID_ContainerFormatPng, (executableDirectory + L"/Screenshots/" + std::to_wstring(time(0)) + L".png").c_str());

		SAFE_RELEASE(tensorTexture);
	}
}

void PointCloudEngine::GroundTruthRenderer::HDF5Draw()
{
	// Clear the render target
	float backgroundColor[4] = { 0.5f, 0.5f, 0.5f, 0 };
	d3d11DevCon->ClearRenderTargetView(renderTargetView, backgroundColor);

	// Clear the depth/stencil view
	d3d11DevCon->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	// Draw
	Draw();

	// Present the result to the screen
	swapChain->Present(0, 0);
}

void PointCloudEngine::GroundTruthRenderer::HDF5DrawDatasets(HDF5File& hdf5file, const UINT groupIndex)
{
	// Save the viewports in numbered groups with leading zeros
	std::stringstream groupNameStream;
	groupNameStream << std::setw(5) << std::setfill('0') << groupIndex;

	H5::Group group = hdf5file.CreateGroup(groupNameStream.str());

	// When using headlight update the light direction
	if (settings->useHeadlight)
	{
		lightingConstantBufferData.lightDirection = camera->GetForward();
		d3d11DevCon->UpdateSubresource(lightingConstantBuffer, 0, NULL, &lightingConstantBufferData, 0, 0);
	}

	// Calculates view and projection matrices and sets the viewport
	camera->PrepareDraw();

	// Draw and present in four different view modes
	for (UINT i = 0; i < 4; i++)
	{
		settings->viewMode = i;

		// Draw and save the color texture
		HDF5Draw();
		hdf5file.AddColorTextureDataset(group, hdf5DatasetNames[i][0], backBufferTexture);

		// Draw and save normal texture
		constantBufferData.drawNormals = true;
		constantBufferData.normalsInScreenSpace = false;
		HDF5Draw();
		hdf5file.AddColorTextureDataset(group, hdf5DatasetNames[i][1], backBufferTexture);

		// Draw screen space normal texture
		constantBufferData.normalsInScreenSpace = true;
		HDF5Draw();
		constantBufferData.drawNormals = false;
		hdf5file.AddColorTextureDataset(group, hdf5DatasetNames[i][2], backBufferTexture);

		// Draw depth again without blending (depth buffer is cleared when using blending)
		if (i < 2 && settings->useBlending)
		{
			settings->useBlending = false;
			HDF5Draw();
			settings->useBlending = true;
		}

		// Save depth texture
		hdf5file.AddDepthTextureDataset(group, hdf5DatasetNames[i][3], depthStencilTexture);
	}

	group.close();
}

void PointCloudEngine::GroundTruthRenderer::GenerateSphereDataset(HDF5File& hdf5file)
{
	Vector3 center = boundingCubePosition * sceneObject->transform->scale;
	float r = Vector3::Distance(camera->GetPosition(), center);

	UINT counter = 0;
	float h = settings->sphereStepSize;

	for (float theta = settings->sphereMinTheta + h / 2; theta < settings->sphereMaxTheta; theta += h / 2)
	{
		for (float phi = settings->sphereMinPhi + h; phi < settings->sphereMaxPhi; phi += h)
		{
			// Rotate around and look at the center
			camera->SetPosition(center + r * Vector3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta)));
			camera->LookAt(center);

			HDF5DrawDatasets(hdf5file, counter++);
		}
	}
}

void PointCloudEngine::GroundTruthRenderer::GenerateWaypointDataset(HDF5File& hdf5file)
{
	WaypointRenderer* waypointRenderer = sceneObject->GetComponent<WaypointRenderer>();

	if (waypointRenderer != NULL)
	{
		UINT counter = 0;
		float waypointLocation = 0;
		Vector3 newCameraPosition = camera->GetPosition();
		Matrix newCameraRotation = camera->GetRotationMatrix();

		while (waypointRenderer->LerpWaypoints(waypointLocation, newCameraPosition, newCameraRotation))
		{
			camera->SetPosition(newCameraPosition);
			camera->SetRotationMatrix(newCameraRotation);

			HDF5DrawDatasets(hdf5file, counter++);

			waypointLocation += settings->waypointStepSize;
		}
	}
}
