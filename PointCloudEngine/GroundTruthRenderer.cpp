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

	// Select the screen area of the neural network compared to the splats
	if (Input::GetKey(Keyboard::Up))
	{
		settings->neuralNetworkScreenArea += 0.5f * dt;
	}
	else if (Input::GetKey(Keyboard::Down))
	{
		settings->neuralNetworkScreenArea -= 0.5f * dt;
	}

	settings->neuralNetworkScreenArea = min(max(0.0f, settings->neuralNetworkScreenArea), 1.0f);

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
	// Evaluate neural network and present the result to the screen
	if (settings->viewMode == 4)
	{
		DrawNeuralNetwork();
		return;
	}
	else if (settings->viewMode < 2)
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
}

void GroundTruthRenderer::Release()
{
    SAFE_RELEASE(vertexBuffer);
    SAFE_RELEASE(constantBuffer);

	// Neural Network
	SAFE_RELEASE(colorTexture);
	SAFE_RELEASE(depthTexture);
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
		helpTextRenderer->text.append(L"[UP/DOWN] Increase/decrease neural network screen area\n");
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
		textRenderer->text.append(L"Neural Network Screen Area: " + std::to_wstring(settings->neuralNetworkScreenArea) + L"\n");
		textRenderer->text.append(L"L1 Loss: " + ((settings->neuralNetworkScreenArea < 1.0f) ? std::to_wstring(l1Loss) : L"Off") + L"\n");
		textRenderer->text.append(L"Mean Square Error Loss: " + ((settings->neuralNetworkScreenArea < 1.0f) ? std::to_wstring(mseLoss) : L"Off") + L"\n");
		textRenderer->text.append(L"Smooth L1 Loss: " + ((settings->neuralNetworkScreenArea < 1.0f) ? std::to_wstring(smoothL1Loss) : L"Off") + L"\n");
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
	if (loadPytorchModel)
	{
		// Only do this once
		loadPytorchModel = false;

		const std::wstring modelFilename = executableDirectory + L"\\NeuralNetwork.pt";
		const std::wstring modelDescriptionFilename = executableDirectory + L"\\NeuralNetworkDescription.txt";

		// Load .txt file storing neural network input and output channel descriptions
		// Each entry consists of:
		// - String: Name of the channel (render mode)
		// - Int: Dimension of channel
		// - String: Identifying if the channel is input (inp) or output (tar)
		// - String: Transformation keywords e.g. normalization
		// - Int: Offset of this channel from the start channel
		std::wifstream modelDescriptionFile(executableDirectory + L"\\NeuralNetworkDescription.txt");

		if (modelDescriptionFile.is_open())
		{
			std::wstring line;

			if (std::getline(modelDescriptionFile, line))
			{
				std::wstring tmp = L"";

				// Remove unwanted characters
				for (int i = 0; i < line.length(); i++)
				{
					wchar_t c = line.at(i);

					if (c != ' ' && c != L'\'' && c != L'[' && c != L']')
					{
						tmp += c;
					}
				}

				line = tmp;

				std::vector<std::wstring> splits = SplitString(line, L',');

				// Parse the content from the split into a struct
				for (int i = 0; i < splits.size(); i += 5)
				{
					ModelChannel channel;
					channel.name = splits[i];
					channel.dimensions = std::stoi(splits[i + 1]);
					channel.offset = std::stoi(splits[i + 4]);
					channel.input = !std::wcscmp(splits[i + 2].c_str(), L"inp");
					channel.normalize = !std::wcscmp(splits[i + 3].c_str(), L"normalize");

					// Sum up to get the total dimensions of the input and output
					if (channel.input)
					{
						inputDimensions += channel.dimensions;
					}
					else
					{
						outputDimensions += channel.dimensions;
					}

					modelChannels.push_back(channel);
				}
			}
			else
			{
				ERROR_MESSAGE(L"Could not parse Neural Network Description file " + modelDescriptionFilename);
				return;
			}
		}
		else
		{
			ERROR_MESSAGE(L"Could not open Neural Network Description file " + modelDescriptionFilename);
			return;
		}

		// Create CPU readable and writeable textures
		D3D11_TEXTURE2D_DESC colorTextureDesc;
		D3D11_TEXTURE2D_DESC depthTextureDesc;

		// Copy the format information and set the flags
		backBufferTexture->GetDesc(&colorTextureDesc);
		depthStencilTexture->GetDesc(&depthTextureDesc);
		colorTextureDesc.CPUAccessFlags = depthTextureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
		colorTextureDesc.Usage = depthTextureDesc.Usage = D3D11_USAGE_STAGING;
		colorTextureDesc.BindFlags = depthTextureDesc.BindFlags = 0;

		hr = d3d11Device->CreateTexture2D(&colorTextureDesc, NULL, &colorTexture);
		ERROR_MESSAGE_ON_FAIL(hr, NAMEOF(d3d11Device->CreateTexture2D) + L" failed!");

		hr = d3d11Device->CreateTexture2D(&depthTextureDesc, NULL, &depthTexture);
		ERROR_MESSAGE_ON_FAIL(hr, NAMEOF(d3d11Device->CreateTexture2D) + L" failed!");

		// Create an input tensor for the neural network
		inputTensor = torch::zeros({ 1, inputDimensions, settings->resolutionX, settings->resolutionY }, torch::dtype(torch::kFloat32));

		// Create an output tensor for the neural network
		outputTensor = torch::zeros({ 1, outputDimensions, settings->resolutionX, settings->resolutionY }, torch::dtype(torch::kFloat32));

		try
		{
			// Load the neural network from file
			if (settings->useCUDA && torch::cuda::is_available())
			{
				model = torch::jit::load(std::string(modelFilename.begin(), modelFilename.end()), torch::Device(at::kCUDA));
			}
			else
			{
				model = torch::jit::load(std::string(modelFilename.begin(), modelFilename.end()), torch::Device(at::kCPU));
			}

			validPytorchModel = true;
		}
		catch (const std::exception &e)
		{
			ERROR_MESSAGE(L"Could not load Pytorch Jit Neural Network from file " + modelFilename);
		}
	}
	else
	{
		// Copy the renderings to the tensors of the different input channels
		for (auto it = modelChannels.begin(); it != modelChannels.end(); it++)
		{
			if (it->input && (renderModes.find(it->name) != renderModes.end()))
			{
				// Create all the empty input tensors that are used by the channels
				if (renderModes[it->name].y == 1)
				{
					// Depth tensor to use as depth input
					it->tensor = torch::zeros({ it->dimensions, settings->resolutionX, settings->resolutionY }, torch::dtype(torch::kFloat32));
				}
				else
				{
					// Color tensor to use as color texture read/write input and output (D3D11 texture memory layout is different from tensor)
					it->tensor = torch::zeros({ it->dimensions, settings->resolutionX, settings->resolutionY }, torch::dtype(torch::kHalf));
				}

				RenderToTensor(it->name, it->tensor);

				// Move tensors to gpu for faster computation
				if (settings->useCUDA && torch::cuda::is_available())
				{
					it->tensor = it->tensor.cuda();
					inputTensor = inputTensor.cuda();
				}

				if (it->normalize)
				{
					// TODO: Normalize the tensor
					// Right now this is not needed
				}

				// Convert to correct data type and shape for the input
				it->tensor = it->tensor.to(torch::dtype(torch::kFloat32));

				// Assign this channel as input
				for (int i = 0; i < it->dimensions; i++)
				{
					inputTensor[0][it->offset + i] = it->tensor[i];
				}
			}
		}

		if (validPytorchModel)
		{
			try
			{
				std::vector<torch::jit::IValue> inputs;
				inputs.push_back(inputTensor);

				// Evaluate the model, input and output channels are given in the model description
				outputTensor = model.forward(inputs).toTensor();

				// Assign the channel output tensors
				for (auto it = modelChannels.begin(); it != modelChannels.end(); it++)
				{
					if (!it->input)
					{
						it->tensor = outputTensor[0].narrow(0, it->offset, it->dimensions);
					}
				}
			}
			catch (std::exception & e)
			{
				ERROR_MESSAGE(L"Could not evaluate Pytorch Jit Model.\nMake sure that the input dimensions and the resolution is correct!");
			}
		}

		if (settings->neuralNetworkScreenArea >= 0.99f)
		{
			// Fill a four channel color tensor with the output (required due to the texture memory layout)
			torch::Tensor colorTensor = torch::zeros({ 4, settings->resolutionX, settings->resolutionY }, torch::dtype(torch::kHalf));

			// When a value is out of range this channel will be skipped
			int colorToOutputChannels[] =
			{
				settings->neuralNetworkOutputRed,
				settings->neuralNetworkOutputGreen,
				settings->neuralNetworkOutputBlue
			};

			// Select individual output channels based on the values in the settings
			for (int i = 0; i < 3; i++)
			{
				if (colorToOutputChannels[i] >= 0 && colorToOutputChannels[i] < outputDimensions)
				{
					colorTensor[i] = outputTensor[0][colorToOutputChannels[i]];
				}
			}

			colorTensor = colorTensor.permute({ 1, 2, 0 }).contiguous().cpu();

			// Copy the tensor data to the texture
			D3D11_MAPPED_SUBRESOURCE subresource;
			d3d11DevCon->Map(colorTexture, 0, D3D11_MAP_WRITE, 0, &subresource);
			memcpy(subresource.pData, colorTensor.data_ptr(), sizeof(short) * colorTensor.numel());
			d3d11DevCon->Unmap(colorTexture, 0);

			// Replace the backbuffer texture by the output
			d3d11DevCon->CopyResource(backBufferTexture, colorTexture);
		}
		else
		{
			CalculateLosses();
		}
	}
}

void PointCloudEngine::GroundTruthRenderer::CalculateLosses()
{
	// Check if the target channel exists
	ModelChannel* targetChannel = NULL;

	for (auto it = modelChannels.begin(); it != modelChannels.end(); it++)
	{
		if (!it->input && !it->name.compare(settings->lossCalculationTarget))
		{
			targetChannel = &*it;
			break;
		}
	}

	// Show an error message and the possible values
	if (targetChannel == NULL)
	{
		std::wstring possibleValues = L"";

		for (auto it = modelChannels.begin(); it != modelChannels.end(); it++)
		{
			if (!it->input)
			{
				possibleValues += it->name + L"\n";
			}
		}

		ERROR_MESSAGE(NAMEOF(settings->lossCalculationTarget) + L" is not valid!\nPossible values are:\n\n" + possibleValues);
		return;
	}

	// Check if the self channel exists
	bool validSelfChannel = renderModes.find(settings->lossCalculationSelf) != renderModes.end();

	// Show an error message and the possible values
	if (!validSelfChannel)
	{
		std::wstring possibleValues = L"";

		for (auto it = renderModes.begin(); it != renderModes.end(); it++)
		{
			possibleValues += it->first + L"\n";
		}

		ERROR_MESSAGE(NAMEOF(settings->lossCalculationSelf) + L" is not valid!\nPossible values are:\n\n" + possibleValues);
		return;
	}

	// Tensors that are used for the loss calculation and the display to the screen
	torch::Tensor colorTensor = torch::zeros({ 4, settings->resolutionX, settings->resolutionY }, torch::dtype(torch::kHalf));
	torch::Tensor selfTensor = torch::zeros_like(targetChannel->tensor);

	// Render the loss input and store it in the self tensor
	RenderToTensor(settings->lossCalculationSelf, selfTensor);

	// Calculate the losses
	l1Loss = torch::l1_loss(selfTensor, targetChannel->tensor).cpu().data<float>()[0];
	mseLoss = torch::mse_loss(selfTensor, targetChannel->tensor).cpu().data<float>()[0];
	smoothL1Loss = torch::smooth_l1_loss(selfTensor, targetChannel->tensor).cpu().data<float>()[0];

	// Convert into correct data type
	selfTensor = selfTensor.to(torch::dtype(torch::kHalf)).cpu();
	targetChannel->tensor = targetChannel->tensor.to(torch::dtype(torch::kHalf)).cpu();

	// Copy parts of the corresponding channels into the color tensor
	for (int i = 0; i < targetChannel->dimensions; i++)
	{
		size_t numTarget = settings->neuralNetworkScreenArea * colorTensor[i].numel();
		size_t numSelf = colorTensor[i].numel() - numTarget;
		memcpy(colorTensor[i].data_ptr(), selfTensor[i].data_ptr(), sizeof(short) * numSelf);
		memcpy((short*)colorTensor[i].data_ptr() + numSelf, (short*)targetChannel->tensor[i].data_ptr() + numSelf, sizeof(short) * numTarget);
	}

	// Convert the color tensor into DirextX texture memory layout
	colorTensor = colorTensor.permute({ 1, 2, 0 }).contiguous();

	// Copy the color data to the texture
	D3D11_MAPPED_SUBRESOURCE subresource;
	d3d11DevCon->Map(colorTexture, 0, D3D11_MAP_WRITE, 0, &subresource);
	memcpy(subresource.pData, colorTensor.data_ptr(), sizeof(short) * colorTensor.numel());
	d3d11DevCon->Unmap(colorTexture, 0);

	// Show results on screen
	d3d11DevCon->CopyResource(backBufferTexture, colorTexture);
}

void PointCloudEngine::GroundTruthRenderer::RenderToTensor(std::wstring renderMode, torch::Tensor& tensor)
{
	// Set the view mode according to the channel
	settings->viewMode = renderModes[renderMode].x;

	// Render differently based on the shading mode
	switch (renderModes[renderMode].y)
	{
		case 0:
		{
			// Color
			Redraw(false);
			CopyBackbufferTextureToTensor(tensor);
			break;
		}
		case 1:
		{
			// Depth without blending (depth buffer is cleared when using blending)
			if (settings->useBlending)
			{
				settings->useBlending = false;
				Redraw(false);
				settings->useBlending = true;
			}
			else
			{
				Redraw(false);
			}

			CopyDepthTextureToTensor(tensor);
			break;
		}
		case 2:
		{
			// Normal
			constantBufferData.drawNormals = true;
			constantBufferData.normalsInScreenSpace = false;
			Redraw(false);
			constantBufferData.drawNormals = false;
			CopyBackbufferTextureToTensor(tensor);
			break;
		}
		case 3:
		{
			// Normal Screen
			constantBufferData.drawNormals = true;
			constantBufferData.normalsInScreenSpace = true;
			Redraw(false);
			constantBufferData.drawNormals = false;
			CopyBackbufferTextureToTensor(tensor);
			break;
		}
	}

	// Reset to the neural network view mode
	settings->viewMode = 4;
}

void PointCloudEngine::GroundTruthRenderer::CopyBackbufferTextureToTensor(torch::Tensor& tensor)
{
	d3d11DevCon->CopyResource(colorTexture, backBufferTexture);

	// This temporary tensor is required due to the memory layout of the texture
	torch::Tensor colorTensor = torch::zeros({ settings->resolutionX, settings->resolutionY, 4 }, torch::dtype(torch::kHalf));

	// Copy the data from the color texture to the cpu tensor
	D3D11_MAPPED_SUBRESOURCE subresource;
	d3d11DevCon->Map(colorTexture, 0, D3D11_MAP_READ, 0, &subresource);
	memcpy(colorTensor.data_ptr(), subresource.pData, sizeof(short) * colorTensor.numel());
	d3d11DevCon->Unmap(colorTexture, 0);

	colorTensor = colorTensor.permute({ 2, 0, 1 }).contiguous();

	for (int i = 0; i < tensor.size(0); i++)
	{
		tensor[i] = colorTensor[i];
	}
}

void PointCloudEngine::GroundTruthRenderer::CopyDepthTextureToTensor(torch::Tensor& tensor)
{
	d3d11DevCon->CopyResource(depthTexture, depthStencilTexture);

	// Make sure that the format matches
	tensor = tensor.to(torch::dtype(torch::kFloat32)).cpu();

	// Copy the data from the depth texture to the cpu tensor
	D3D11_MAPPED_SUBRESOURCE subresource;
	d3d11DevCon->Map(depthTexture, 0, D3D11_MAP_READ, 0, &subresource);
	memcpy(tensor.data_ptr(), subresource.pData, sizeof(float) * tensor.numel());
	d3d11DevCon->Unmap(depthTexture, 0);
}

void PointCloudEngine::GroundTruthRenderer::OutputTensorSize(torch::Tensor &tensor)
{
	c10::IntArrayRef sizes = tensor.sizes();

	for (int i = 0; i < sizes.size(); i++)
	{
		OutputDebugString((std::to_wstring(sizes[i]) + L" ").c_str());
	}

	OutputDebugString(L"\n");
}

void PointCloudEngine::GroundTruthRenderer::Redraw(bool present)
{
	// Clear the render target and depth/stencil view
	d3d11DevCon->ClearRenderTargetView(renderTargetView, (float*)&settings->backgroundColor);
	d3d11DevCon->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	Draw();

	if (present)
	{
		// Present the result to the screen
		swapChain->Present(0, 0);
	}
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

	// Draw in every render mode and save it to the dataset
	for (auto it = renderModes.begin(); it != renderModes.end(); it++)
	{
		settings->viewMode = it->second.x;

		// Render differently based on the shading mode
		switch (it->second.y)
		{
			case 0:
			{
				// Color
				Redraw(true);
				hdf5file.AddColorTextureDataset(group, it->first, backBufferTexture);
				break;
			}
			case 1:
			{
				// Depth without blending (depth buffer is cleared when using blending)
				if (settings->useBlending)
				{
					settings->useBlending = false;
					Redraw(true);
					settings->useBlending = true;
				}
				else
				{
					Redraw(true);
				}
				hdf5file.AddDepthTextureDataset(group, it->first, depthStencilTexture);
				break;
			}
			case 2:
			{
				// Normal
				constantBufferData.drawNormals = true;
				constantBufferData.normalsInScreenSpace = false;
				Redraw(true);
				constantBufferData.drawNormals = false;
				hdf5file.AddColorTextureDataset(group, it->first, backBufferTexture);
				break;
			}
			case 3:
			{
				// Normal Screen
				constantBufferData.drawNormals = true;
				constantBufferData.normalsInScreenSpace = true;
				Redraw(true);
				constantBufferData.drawNormals = false;
				hdf5file.AddColorTextureDataset(group, it->first, backBufferTexture);
				break;
			}
		}
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

std::vector<std::wstring> PointCloudEngine::GroundTruthRenderer::SplitString(std::wstring s, wchar_t delimiter)
{
	std::vector<std::wstring> output;
	size_t index = s.find_first_of(delimiter);

	while (index >= 0 && index <= s.length())
	{
		// Add the left side of the split to the output
		output.push_back(s.substr(0, index));

		// Continue looking for splits in the right side
		s = s.substr(index + 1, s.length());
		index = s.find_first_of(delimiter);
	}

	output.push_back(s);

	return output;
}
