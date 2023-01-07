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
	ERROR_MESSAGE_ON_HR(hr, NAMEOF(d3d11Device->CreateBuffer) + L" failed for the " + NAMEOF(vertexBuffer));

    // Create the constant buffer for WVP
    D3D11_BUFFER_DESC constantBufferDesc;
    ZeroMemory(&constantBufferDesc, sizeof(constantBufferDesc));
	constantBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	constantBufferDesc.ByteWidth = sizeof(GroundTruthRendererConstantBuffer);
	constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	constantBufferDesc.CPUAccessFlags = 0;
	constantBufferDesc.MiscFlags = 0;

    hr = d3d11Device->CreateBuffer(&constantBufferDesc, NULL, &constantBuffer);
	ERROR_MESSAGE_ON_HR(hr, NAMEOF(d3d11Device->CreateBuffer) + L" failed for the " + NAMEOF(constantBuffer));
}

void GroundTruthRenderer::Update()
{
	// Release pull push resources if no longer needed
	if (settings->viewMode != ViewMode::PullPush)
	{
		SAFE_RELEASE(pullPush);
	}
}

void GroundTruthRenderer::Draw()
{
	// Evaluate neural network and present the result to the screen
	if (settings->viewMode == ViewMode::NeuralNetwork)
	{
		DrawNeuralNetwork();
		return;
	}
	else if (settings->viewMode == ViewMode::Splats || settings->viewMode == ViewMode::SparseSplats)
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
	constantBufferData.backfaceCulling = settings->backfaceCulling;
    constantBufferData.cameraPosition = camera->GetPosition();
	constantBufferData.blendFactor = settings->blendFactor;
	constantBufferData.useBlending = false;
	constantBufferData.shadingMode = (int)settings->shadingMode;

	// The amount of points that will be drawn
	UINT vertexCount = vertices.size();

	// Set different sampling rates based on the view mode
	if (settings->viewMode == ViewMode::Splats)
	{
		constantBufferData.samplingRate = settings->samplingRate;
	}
	else if (settings->viewMode == ViewMode::SparseSplats || settings->viewMode == ViewMode::SparsePoints)
	{
		constantBufferData.samplingRate = settings->sparseSamplingRate;

		// Only draw a portion of the point cloud to simulate the selected density
		// This requires the vertex indices to be distributed randomly (pointcloud files provide this feature)
		vertexCount *= settings->density;
	}
	else if (settings->viewMode == ViewMode::PullPush)
	{
		// In case of the pull push algorithm this is the initial point sample count at the full resolution layer
		vertexCount *= settings->density;
	}

    // Update effect file buffer, set shader buffer to our created buffer
    d3d11DevCon->UpdateSubresource(constantBuffer, 0, NULL, &constantBufferData, 0, 0);
	d3d11DevCon->VSSetConstantBuffers(0, 1, &constantBuffer);
	d3d11DevCon->GSSetConstantBuffers(0, 1, &constantBuffer);
	d3d11DevCon->PSSetConstantBuffers(0, 1, &constantBuffer);

	if (settings->useBlending && (settings->viewMode == ViewMode::Splats || settings->viewMode == ViewMode::SparseSplats) && (settings->shadingMode != ShadingMode::Depth))
	{
		DrawBlended(vertexCount, constantBuffer, &constantBufferData, constantBufferData.useBlending);
	}
	else
	{
		d3d11DevCon->Draw(vertexCount, 0);

		if (settings->viewMode == ViewMode::PullPush)
		{
			if (pullPush == NULL)
			{
				pullPush = new PullPush();
			}

			d3d11DevCon->CSSetConstantBuffers(0, 1, &constantBuffer);
			pullPush->SetInitialColorTexture(backBufferTexture);

			constantBufferData.shadingMode = (int)ShadingMode::Normal;
			d3d11DevCon->UpdateSubresource(constantBuffer, 0, NULL, &constantBufferData, 0, 0);
			d3d11DevCon->ClearRenderTargetView(renderTargetView, (float*)&settings->backgroundColor);
			d3d11DevCon->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
			d3d11DevCon->Draw(vertexCount, 0);
			constantBufferData.shadingMode = (int)settings->shadingMode;

			pullPush->SetInitialNormalTexture(backBufferTexture);
			pullPush->Execute(backBufferTexture, depthTextureSRV);
		}
	}

	// Show vertex count on GUI
	GUI::vertexCount = vertexCount;
}

void GroundTruthRenderer::Release()
{
    SAFE_RELEASE(vertexBuffer);
    SAFE_RELEASE(constantBuffer);
	SAFE_RELEASE(pullPush);

#ifndef IGNORE_OLD_PYTORCH_AND_HDF5_IMPLEMENTATION
	// Neural Network
	SAFE_RELEASE(colorTexture);
	SAFE_RELEASE(depthTexture);
#endif
}

void PointCloudEngine::GroundTruthRenderer::GetBoundingCubePositionAndSize(Vector3 &outPosition, float &outSize)
{
	outPosition = boundingCubePosition;
	outSize = boundingCubeSize;
}

void PointCloudEngine::GroundTruthRenderer::RemoveComponentFromSceneObject()
{
	sceneObject->RemoveComponent(this);
}

Component* PointCloudEngine::GroundTruthRenderer::GetComponent()
{
	return this;
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

void PointCloudEngine::GroundTruthRenderer::DrawNeuralNetwork()
{
	// TODO
	// - Draw point depth, color, normal screen, optical flow
	// - Convert to pytorch tensors
	// - Evaluate neural network
	// - Copy result based on shading mode back to backbuffer
	// - Present
}

#ifndef IGNORE_OLD_PYTORCH_AND_HDF5_IMPLEMENTATION
void PointCloudEngine::GroundTruthRenderer::GenerateSphereDataset()
{
	HDF5File hdf5file = CreateDatasetHDF5File();
	ViewMode startViewMode = settings->viewMode;
	Vector3 startPosition = camera->GetPosition();
	Matrix startRotation = camera->GetRotationMatrix();

	Vector3 center = boundingCubePosition * sceneObject->transform->scale;
	float r = Vector3::Distance(camera->GetPosition(), center);

	UINT counter = 0;
	float h = settings->sphereStepSize;

	// Clear and add the GUI camera records
	GUI::cameraRecordingPositions.clear();
	GUI::cameraRecordingRotations.clear();

	for (float theta = settings->sphereMinTheta + h / 2; theta < settings->sphereMaxTheta; theta += h / 2)
	{
		for (float phi = settings->sphereMinPhi + h; phi < settings->sphereMaxPhi; phi += h)
		{
			// Rotate around and look at the center
			Vector3 newPosition = center + r * Vector3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
			camera->SetPosition(newPosition);
			camera->LookAt(center);
			GUI::cameraRecordingPositions.push_back(newPosition);
			GUI::cameraRecordingRotations.push_back(camera->GetRotationMatrix());

			HDF5DrawDatasets(hdf5file, counter++);
		}
	}

	// Reset properties
	camera->SetPosition(startPosition);
	camera->SetRotationMatrix(startRotation);
	settings->viewMode = startViewMode;
}

void PointCloudEngine::GroundTruthRenderer::GenerateWaypointDataset()
{
	HDF5File hdf5file = CreateDatasetHDF5File();
	ViewMode startViewMode = settings->viewMode;
	Vector3 startPosition = camera->GetPosition();
	Matrix startRotation = camera->GetRotationMatrix();

	WaypointRenderer* waypointRenderer = sceneObject->GetComponent<WaypointRenderer>();

	// Clear and add the GUI camera records
	GUI::cameraRecordingPositions.clear();
	GUI::cameraRecordingRotations.clear();

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
			GUI::cameraRecordingPositions.push_back(newCameraPosition);
			GUI::cameraRecordingRotations.push_back(newCameraRotation);

			HDF5DrawDatasets(hdf5file, counter++);

			waypointLocation += settings->waypointStepSize;
		}
	}

	// Reset properties
	camera->SetPosition(startPosition);
	camera->SetRotationMatrix(startRotation);
	settings->viewMode = startViewMode;

}

void PointCloudEngine::GroundTruthRenderer::LoadNeuralNetworkPytorchModel()
{
	// Set this to false in any error case
	validPytorchModel = true;

	try
	{
		// Load the neural network from file
		if (settings->useCUDA && torch::cuda::is_available())
		{
			model = torch::jit::load(std::string(settings->neuralNetworkModelFile.begin(), settings->neuralNetworkModelFile.end()), torch::Device(at::kCUDA));
		}
		else
		{
			model = torch::jit::load(std::string(settings->neuralNetworkModelFile.begin(), settings->neuralNetworkModelFile.end()), torch::Device(at::kCPU));
		}
	}
	catch (const std::exception & e)
	{
		ERROR_MESSAGE(L"Could not load Pytorch Jit Neural Network from file " + settings->neuralNetworkModelFile);
		validPytorchModel = false;
		return;
	}

	createInputOutputTensors = true;
}

void PointCloudEngine::GroundTruthRenderer::LoadNeuralNetworkDescriptionFile()
{
	// Set this to false in any error case
	validDescriptionFile = true;

	// Load .txt file storing neural network input and output channel descriptions
	// Each entry consists of:
	// - String: Name of the channel (render mode)
	// - Int: Dimension of channel
	// - String: Identifying if the channel is input (inp) or output (tar)
	// - String: Transformation keywords e.g. normalization
	// - Int: Offset of this channel from the start channel
	std::wifstream modelDescriptionFile(settings->neuralNetworkDescriptionFile);

	if (!modelDescriptionFile.is_open())
	{
		ERROR_MESSAGE(L"Could not open Neural Network Description file " + settings->neuralNetworkDescriptionFile);
		validDescriptionFile = false;
		return;
	}

	try
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
			modelChannels.clear();
			inputDimensions = 0;
			outputDimensions = 0;
			std::vector<std::wstring> splits = SplitString(line, L',');

			// For GUI
			std::vector<std::wstring> outputChannels;
			std::vector<std::wstring> lossTargetChannels;

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
					lossTargetChannels.push_back(channel.name);

					for (int j = 0; j < channel.dimensions; j++)
					{
						outputChannels.push_back(channel.name + std::to_wstring(j));
					}
				}

				modelChannels.push_back(channel);
			}

			GUI::SetNeuralNetworkOutputChannels(outputChannels);
			GUI::SetNeuralNetworkLossSelfChannels(renderModes);
			GUI::SetNeuralNetworkLossTargetChannels(lossTargetChannels);
		}
	}
	catch (const std::exception &e)
	{
		ERROR_MESSAGE(L"Could not parse Neural Network Description file " + settings->neuralNetworkDescriptionFile);
		validDescriptionFile = false;
		return;
	}

	createInputOutputTensors = true;
}

void PointCloudEngine::GroundTruthRenderer::ApplyNeuralNetworkResolution()
{
	// This is called when the rendering resolution changes
	createInputOutputTensors = true;
	loadPytorchModel = true;
}

void PointCloudEngine::GroundTruthRenderer::DrawNeuralNetwork()
{
	if (loadPytorchModel)
	{
		// Only do this once
		loadPytorchModel = false;
		LoadNeuralNetworkPytorchModel();
		LoadNeuralNetworkDescriptionFile();
	}
	else if (validPytorchModel && validDescriptionFile)
	{
		if (createInputOutputTensors)
		{
			createInputOutputTensors = false;

			// (Re)create CPU readable and writeable textures
			SAFE_RELEASE(colorTexture);
			SAFE_RELEASE(depthTexture);

			D3D11_TEXTURE2D_DESC colorTextureDesc;
			backBufferTexture->GetDesc(&colorTextureDesc);
			colorTextureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
			colorTextureDesc.Usage = D3D11_USAGE_STAGING;
			colorTextureDesc.BindFlags = 0;

			hr = d3d11Device->CreateTexture2D(&colorTextureDesc, NULL, &colorTexture);
			ERROR_MESSAGE_ON_HR(hr, NAMEOF(d3d11Device->CreateTexture2D) + L" failed!");

			D3D11_TEXTURE2D_DESC depthTextureDesc;
			depthStencilTexture->GetDesc(&depthTextureDesc);
			depthTextureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
			depthTextureDesc.Usage = D3D11_USAGE_STAGING;
			depthTextureDesc.BindFlags = 0;

			hr = d3d11Device->CreateTexture2D(&depthTextureDesc, NULL, &depthTexture);
			ERROR_MESSAGE_ON_HR(hr, NAMEOF(d3d11Device->CreateTexture2D) + L" failed!");

			// Create an input tensor for the neural network
			inputTensor = torch::zeros({ 1, inputDimensions, settings->resolutionX, settings->resolutionY }, torch::dtype(torch::kFloat32));

			// Create an output tensor for the neural network
			outputTensor = torch::zeros({ 1, outputDimensions, settings->resolutionX, settings->resolutionY }, torch::dtype(torch::kFloat32));
		}

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
			validPytorchModel = false;
			return;
		}

		if (GUI::lossFunctionSelection == 0)
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
		validPytorchModel = false;
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
		validPytorchModel = false;
		return;
	}

	// Tensors that are used for the loss calculation and the display to the screen
	torch::Tensor colorTensor = torch::zeros({ 4, settings->resolutionX, settings->resolutionY }, torch::dtype(torch::kHalf));
	torch::Tensor selfTensor = torch::zeros_like(targetChannel->tensor);

	// Render the loss input and store it in the self tensor
	RenderToTensor(settings->lossCalculationSelf, selfTensor);

	// Move tensors to gpu for faster computation
	if (settings->useCUDA && torch::cuda::is_available())
	{
		selfTensor = selfTensor.cuda();
		targetChannel->tensor = targetChannel->tensor.cuda();
	}

	// Calculate the losses
	GUI::l1Loss = torch::l1_loss(selfTensor, targetChannel->tensor).cpu().data<float>()[0];
	GUI::mseLoss = torch::mse_loss(selfTensor, targetChannel->tensor).cpu().data<float>()[0];
	GUI::smoothL1Loss = torch::smooth_l1_loss(selfTensor, targetChannel->tensor).cpu().data<float>()[0];

	// Convert into correct data type
	selfTensor = selfTensor.to(torch::dtype(torch::kHalf)).cpu();
	targetChannel->tensor = targetChannel->tensor.to(torch::dtype(torch::kHalf)).cpu();

	// Copy parts of the corresponding channels into the color tensor
	for (int i = 0; i < targetChannel->dimensions; i++)
	{
		size_t numTarget = settings->neuralNetworkLossArea * colorTensor[i].numel();
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
	settings->viewMode = (ViewMode)renderModes[renderMode].x;

	// Render differently based on the shading mode
	switch ((ShadingMode)renderModes[renderMode].y)
	{
		case ShadingMode::Color:
		{
			// Color
			Redraw(false);
			CopyBackbufferTextureToTensor(tensor);
			break;
		}
		case ShadingMode::Depth:
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
		case ShadingMode::Normal:
		{
			// Normal
			constantBufferData.drawNormals = true;
			constantBufferData.normalsInScreenSpace = false;
			Redraw(false);
			constantBufferData.drawNormals = false;
			CopyBackbufferTextureToTensor(tensor);
			break;
		}
		case ShadingMode::NormalScreen:
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
	settings->viewMode = ViewMode::NeuralNetwork;
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
	memcpy(tensor.data_ptr(), subresource.pData, sizeof(float) * settings->resolutionX * settings->resolutionY);
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

	HDF5DrawRenderModes(hdf5file, group, L"");

	// Render again but with lower resolution
	int resolutionX = settings->resolutionX;
	int resolutionY = settings->resolutionY;
	ChangeRenderingResolution(resolutionX / settings->downsampleFactor, resolutionY / settings->downsampleFactor);

	// Also upsample the low resolution rendering with blank pixel padding and save it to the hdf5 file
	// The downsample factor should be a power of 2 to avoid image stretching
	HDF5DrawRenderModes(hdf5file, group, L"LowRes", true);

	// Reset to full resolution
	ChangeRenderingResolution(resolutionX, resolutionY);
}

void PointCloudEngine::GroundTruthRenderer::HDF5DrawRenderModes(HDF5File& hdf5file, H5::Group group, std::wstring comment, bool sparseUpsample)
{
	// Calculates view and projection matrices and sets the viewport
	camera->PrepareDraw();

	// Draw in every render mode and save it to the dataset
	for (auto it = renderModes.begin(); it != renderModes.end(); it++)
	{
		settings->viewMode = (ViewMode)it->second.x;

		// Render differently based on the shading mode
		switch ((ShadingMode)it->second.y)
		{
			case ShadingMode::Color:
			{
				// Color
				Redraw(true);
				hdf5file.AddColorTextureDataset(group, it->first + comment, backBufferTexture, sparseUpsample);
				break;
			}
			case ShadingMode::Depth:
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
				hdf5file.AddDepthTextureDataset(group, it->first + comment, depthStencilTexture, sparseUpsample);
				break;
			}
			case ShadingMode::Normal:
			{
				// Normal
				constantBufferData.drawNormals = true;
				constantBufferData.normalsInScreenSpace = false;
				Redraw(true);
				constantBufferData.drawNormals = false;
				hdf5file.AddColorTextureDataset(group, it->first + comment, backBufferTexture, sparseUpsample);
				break;
			}
			case ShadingMode::NormalScreen:
			{
				// Normal Screen
				constantBufferData.drawNormals = true;
				constantBufferData.normalsInScreenSpace = true;
				Redraw(true);
				constantBufferData.drawNormals = false;
				hdf5file.AddColorTextureDataset(group, it->first + comment, backBufferTexture, sparseUpsample);
				break;
			}
		}
	}

	group.close();
}

HDF5File PointCloudEngine::GroundTruthRenderer::CreateDatasetHDF5File()
{
	// Create a directory for the HDF5 files
	CreateDirectory((executableDirectory + L"/HDF5").c_str(), NULL);

	// Create and save the file
	HDF5File hdf5file(executableDirectory + L"/HDF5/" + std::to_wstring(time(0)) + L".hdf5");

	// Add attribute storing the settings
	hdf5file.AddStringAttribute(L"Settings", settings->ToKeyValueString());

	return hdf5file;
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
#endif