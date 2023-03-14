#include "GroundTruthRenderer.h"

// This is required to avoid issue with torch.min() and torch.max() functions
#ifdef min
	#undef min
#endif

#ifdef max
	#undef max
#endif

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

    // Create the constant buffer (also used by MeshRenderer and PullPush)
    D3D11_BUFFER_DESC constantBufferDesc;
    ZeroMemory(&constantBufferDesc, sizeof(constantBufferDesc));
	constantBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	constantBufferDesc.ByteWidth = sizeof(GroundTruthConstantBuffer);
	constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	constantBufferDesc.CPUAccessFlags = 0;
	constantBufferDesc.MiscFlags = 0;

    hr = d3d11Device->CreateBuffer(&constantBufferDesc, NULL, &constantBuffer);
	ERROR_MESSAGE_ON_HR(hr, NAMEOF(d3d11Device->CreateBuffer) + L" failed for the " + NAMEOF(constantBuffer));

	// Load neural network models
	if (settings->filenameSCM.length() > 0)
	{
		LoadSurfaceClassificationModel();
	}

	if (settings->filenameSFM.length() > 0)
	{
		LoadSurfaceFlowModel();
	}

	if (settings->filenameSRM.length() > 0)
	{
		LoadSurfaceReconstructionModel();
	}

	// Initialize previous output tensor for SRM
	previousOutputSRM = torch::zeros({ 1, 8, settings->resolutionY, settings->resolutionX }, c10::TensorOptions().dtype(torch::kFloat16).device(torch::kCUDA));
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

	// Assign, update and set shader constant buffer variables
	UpdateConstantBuffer();

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
}

void PointCloudEngine::GroundTruthRenderer::UpdateConstantBuffer()
{
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
	constantBufferData.textureLOD = settings->textureLOD;
	constantBufferData.resolutionX = settings->resolutionX;
	constantBufferData.resolutionY = settings->resolutionY;

	// Update effect file buffer, set shader buffer to our created buffer
	d3d11DevCon->UpdateSubresource(constantBuffer, 0, NULL, &constantBufferData, 0, 0);
	d3d11DevCon->VSSetConstantBuffers(0, 1, &constantBuffer);
	d3d11DevCon->GSSetConstantBuffers(0, 1, &constantBuffer);
	d3d11DevCon->PSSetConstantBuffers(0, 1, &constantBuffer);
}

void PointCloudEngine::GroundTruthRenderer::UpdatePreviousMatrices()
{
	// Store the previous matrices for optical flow computation
	constantBufferData.PreviousWorld = constantBufferData.World;
	constantBufferData.PreviousView = constantBufferData.View;
	constantBufferData.PreviousProjection = constantBufferData.Projection;
	constantBufferData.PreviousWorldInverseTranspose = constantBufferData.WorldInverseTranspose;
}

void PointCloudEngine::GroundTruthRenderer::LoadSurfaceClassificationModel()
{
	validSCM = LoadNeuralNetworkModel(settings->filenameSCM, SCM);
}

void PointCloudEngine::GroundTruthRenderer::LoadSurfaceFlowModel()
{
	validSFM = LoadNeuralNetworkModel(settings->filenameSFM, SFM);
}

void PointCloudEngine::GroundTruthRenderer::LoadSurfaceReconstructionModel()
{
	validSRM = LoadNeuralNetworkModel(settings->filenameSRM, SRM);
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
	if (!validSCM || !validSFM || !validSRM)
	{
		std::wstring warningMessage = L"All neural networks models need to be loaded from scripted Pytorch model files before evaluation is possible!";
		warningMessage += L"\n\nMissing files for:\n";

		if (!validSCM)
		{
			warningMessage += L"- Surface Classification Model\n";
		}

		if (!validSFM)
		{
			warningMessage += L"- Surface Flow Model\n";
		}

		if (!validSRM)
		{
			warningMessage += L"- Surface Reconstruction Model\n";
		}

		WARNING_MESSAGE(warningMessage);
		return;
	}

	try
	{
		ShadingMode startShadingMode = settings->shadingMode;

		settings->viewMode = ViewMode::SparsePoints;
		settings->shadingMode = ShadingMode::Color;
		Redraw(false);

		torch::Tensor depthTensor = DXCUDATORCH::GetDepthTensor().permute({ 0, 3, 1, 2 });
		torch::Tensor colorTensor = DXCUDATORCH::GetBackbufferTensor().permute({ 0, 3, 1, 2 });
		colorTensor = colorTensor.index({ at::indexing::Slice(), at::indexing::Slice(0, 3), at::indexing::Slice(), at::indexing::Slice() });

		settings->shadingMode = ShadingMode::NormalScreen;
		Redraw(false);

		torch::Tensor normalTensor = DXCUDATORCH::GetBackbufferTensor().permute({ 0, 3, 1, 2 });
		normalTensor = normalTensor.index({ at::indexing::Slice(), at::indexing::Slice(0, 3), at::indexing::Slice(), at::indexing::Slice() });

		settings->shadingMode = ShadingMode::OpticalFlowForward;
		Redraw(false);

		torch::Tensor opticalFlowForwardTensor = DXCUDATORCH::GetBackbufferTensor().permute({ 0, 3, 1, 2 });
		opticalFlowForwardTensor = opticalFlowForwardTensor.index({ at::indexing::Slice(), at::indexing::Slice(0, 2), at::indexing::Slice(), at::indexing::Slice() });

		// Prepare tensors for model evaluation
		torch::Tensor foregroundMask = depthTensor < 1.0f;
		torch::Tensor backgroundMask = depthTensor >= 1.0f;
		torch::Tensor depthNormalizedTensor = NormalizeDepthTensor(depthTensor, foregroundMask, backgroundMask).to(torch::kFloat16);
		foregroundMask = foregroundMask.to(torch::kFloat16);
		backgroundMask = backgroundMask.to(torch::kFloat16);

		// Surface Classification Model
		torch::Tensor inputSCM = torch::cat({ foregroundMask, depthNormalizedTensor, normalTensor }, 1);
		torch::Tensor outputSCM = EvaluateNeuralNetworkModel(SCM, inputSCM);

		// Apply surface mask
		torch::Tensor sparseSurfaceMask = (outputSCM >= settings->thresholdSCM).to(torch::kFloat16);
		torch::Tensor sparseSurfaceDepth = sparseSurfaceMask * depthNormalizedTensor;
		torch::Tensor sparseSurfaceColor = sparseSurfaceMask * colorTensor;
		torch::Tensor sparseSurfaceNormal = sparseSurfaceMask * normalTensor;
		torch::Tensor sparseSurfaceFlow = sparseSurfaceMask * opticalFlowForwardTensor;

		// Renormalize depth for the sparse surface (since non-surface pixel depth values are now gone)
		torch::Tensor tmpMin, tmpMax;
		std::tie(tmpMin, tmpMax, sparseSurfaceDepth) = ConvertTensorIntoZeroToOneRange(sparseSurfaceDepth);

		// Normalize flow into [0, 1] range
		torch::Tensor sparseSurfaceFlowMin, sparseSurfaceFlowMax, sparseSurfaceFlowNormalized;
		std::tie(sparseSurfaceFlowMin, sparseSurfaceFlowMax, sparseSurfaceFlowNormalized) = ConvertTensorIntoZeroToOneRange(sparseSurfaceFlow);

		// Surface Flow Model
		torch::Tensor inputSFM = torch::cat({ sparseSurfaceMask, sparseSurfaceDepth, sparseSurfaceNormal, sparseSurfaceFlowNormalized }, 1);
		torch::Tensor outputSFM = EvaluateNeuralNetworkModel(SFM, inputSFM);

		// Revert dense flow normalization
		torch::Tensor surfaceFlowSFM = RevertTensorIntoFullRange(sparseSurfaceFlowMin, sparseSurfaceFlowMax, outputSFM);

		// Surface Reconstruction Model
		torch::Tensor inputSRM = torch::cat({ sparseSurfaceMask, sparseSurfaceDepth, sparseSurfaceColor, sparseSurfaceNormal }, 1);

		// Possibly need recreate the previous output tensor when resolution changed
		if ((previousOutputSRM.size(2) != settings->resolutionY) || (previousOutputSRM.size(3) != settings->resolutionX))
		{
			previousOutputSRM = torch::zeros_like(inputSRM);
		}

		// Warp previous output tensor onto current frame
		previousOutputSRM = WarpImage(previousOutputSRM, surfaceFlowSFM);

		// Evaluate SRM
		inputSRM = torch::cat({ inputSRM, previousOutputSRM }, 1);
		torch::Tensor outputSRM = EvaluateNeuralNetworkModel(SRM, inputSRM);

		// Update previous output for next iteration
		previousOutputSRM = outputSRM;

		// Perform sparse input surface keeping
		torch::Tensor surfaceKeepingMask = (outputSCM >= settings->thresholdSurfaceKeeping).to(torch::kFloat16);
		torch::Tensor surfaceKeepingInput = inputSRM.index({ at::indexing::Slice(), at::indexing::Slice(0, 8), at::indexing::Slice(), at::indexing::Slice() });
		outputSRM = surfaceKeepingMask * surfaceKeepingInput + (1.0f - surfaceKeepingMask) * outputSRM;

		// Slice tensors for presentation
		torch::Tensor outputDepthSRM = outputSRM.index({ at::indexing::Slice(), at::indexing::Slice(1, 2), at::indexing::Slice(), at::indexing::Slice() });
		torch::Tensor outputColorSRM = outputSRM.index({ at::indexing::Slice(), at::indexing::Slice(2, 5), at::indexing::Slice(), at::indexing::Slice() });
		torch::Tensor outputNormalSRM = outputSRM.index({ at::indexing::Slice(), at::indexing::Slice(5, 8), at::indexing::Slice(), at::indexing::Slice() });

		long long h = settings->resolutionY;
		long long w = settings->resolutionX;
		torch::Tensor presentTensor = torch::zeros({ 1, 4, h, w }, c10::TensorOptions().device(torch::kCUDA).dtype(torch::kFloat16));

		switch (startShadingMode)
		{
			case ShadingMode::Depth:
			{
				presentTensor.index_put_({ at::indexing::Slice(), at::indexing::Slice(0, 3), at::indexing::Slice(), at::indexing::Slice(0) }, outputDepthSRM.repeat({ 1, 3, 1, 1 }));
				break;
			}
			case ShadingMode::Color:
			{
				presentTensor.index_put_({ at::indexing::Slice(), at::indexing::Slice(0, 3), at::indexing::Slice(), at::indexing::Slice(0) }, outputColorSRM);
				break;
			}
			case ShadingMode::NormalScreen:
			{
				presentTensor.index_put_({ at::indexing::Slice(), at::indexing::Slice(0, 3), at::indexing::Slice(), at::indexing::Slice(0) }, outputNormalSRM);
				break;
			}
			case ShadingMode::OpticalFlowForward:
			{
				presentTensor.index_put_({ at::indexing::Slice(), at::indexing::Slice(0, 2), at::indexing::Slice(), at::indexing::Slice(0) }, outputSFM);
				break;
			}
		}

		presentTensor = presentTensor.permute({ 0, 2, 3, 1 }).contiguous();

		DXCUDATORCH::SetBackbufferFromTensor(presentTensor);

		settings->viewMode = ViewMode::NeuralNetwork;
		settings->shadingMode = startShadingMode;
	}
	catch (const std::exception& e)
	{
		std::string errorMessage = e.what();
		ERROR_MESSAGE(std::wstring(errorMessage.begin(), errorMessage.end()));
	}
}

torch::Tensor PointCloudEngine::GroundTruthRenderer::NormalizeDepthTensor(torch::Tensor& depthTensor, torch::Tensor& foregroundMask, torch::Tensor& backgroundMask)
{
	torch::Tensor depthNormalizedTensor = depthTensor.clone();
	torch::Tensor depthBackground = depthNormalizedTensor.index({ backgroundMask });
	torch::Tensor depthForeground = depthNormalizedTensor.index({ foregroundMask });

	// Only perform normalization if mask is non-empty to avoid NaN error
	if ((depthBackground.numel() > 0) && (depthForeground.numel() > 0))
	{
		// Replace background depth values with 0 for better visualization
		depthNormalizedTensor.index_put_({ backgroundMask }, 0.0f);

		// Compute minimum and maximum masked depth values
		torch::Tensor depthMin = depthForeground.min();
		torch::Tensor depthMax = depthForeground.max();

		// Normalize masked values to range [0, 1]
		depthNormalizedTensor.index_put_({ foregroundMask }, (depthNormalizedTensor.index({ foregroundMask }) - depthMin) / ((depthMax - depthMin) + 1e-12));
	}

	return depthNormalizedTensor;
}

std::tuple<torch::Tensor, torch::Tensor, torch::Tensor> PointCloudEngine::GroundTruthRenderer::ConvertTensorIntoZeroToOneRange(torch::Tensor& tensorFull)
{
	int64_t n = tensorFull.size(0);
	torch::Tensor tensorMin, tensorMinIndices;
	std::tie(tensorMin, tensorMinIndices) = torch::min(tensorFull.reshape({ n, -1 }), 1);
	torch::Tensor tensorZeroOne = tensorFull - tensorMin.view({ n, 1, 1, 1 });
	torch::Tensor tensorMax, tensorMaxIndices;
	std::tie(tensorMax, tensorMaxIndices) = torch::max(tensorZeroOne.reshape({ n, -1 }), 1);
	tensorZeroOne /= tensorMax.view({ n, 1, 1, 1 }) + 1e-12;

	return std::make_tuple(tensorMin, tensorMax, tensorZeroOne);
}

torch::Tensor PointCloudEngine::GroundTruthRenderer::RevertTensorIntoFullRange(torch::Tensor& tensorMin, torch::Tensor& tensorMax, torch::Tensor& tensorZeroOne)
{
	int64_t n = tensorZeroOne.size(0);
	return tensorMin.view({ n, 1, 1, 1 }) + (tensorMax.view({ n, 1, 1, 1 }) * tensorZeroOne);
}

torch::Tensor PointCloudEngine::GroundTruthRenderer::WarpImage(torch::Tensor& imageTensor, torch::Tensor& motionVectorTensor)
{
	// Create a pixel grid tensor that contains pixel coorinates
	torch::Tensor pixelPositionsVertical = torch::arange(0, settings->resolutionY, c10::TensorOptions().dtype(torch::kFloat16).device(torch::kCUDA));
	torch::Tensor pixelPositionsHorizontal = torch::arange(0, settings->resolutionX, c10::TensorOptions().dtype(torch::kFloat16).device(torch::kCUDA));
	pixelPositionsHorizontal = pixelPositionsHorizontal.unsqueeze(0).repeat({ settings->resolutionY, 1 });
	pixelPositionsVertical = pixelPositionsVertical.unsqueeze(0).reshape({ -1, 1 }).repeat({ 1, settings->resolutionX });

	torch::Tensor pixelGrid = torch::stack({ pixelPositionsHorizontal, pixelPositionsVertical }, 0).unsqueeze(0);
	pixelGrid += 0.5f;

	// Apply the motion vector
	torch::Tensor warpGrid = pixelGrid + motionVectorTensor;

	// Normalize into(-1, 1) range for grid_sample
	torch::Tensor widthTensor = torch::full({ warpGrid.size(0), 1, 1, 1 }, settings->resolutionX, c10::TensorOptions().dtype(torch::kFloat16).device(torch::kCUDA));
	torch::Tensor heightTensor = torch::full({ warpGrid.size(0), 1, 1, 1 }, settings->resolutionY, c10::TensorOptions().dtype(torch::kFloat16).device(torch::kCUDA));
	warpGrid /= torch::cat({ widthTensor, heightTensor }, 1);
	warpGrid = (warpGrid * 2.0f) - 1.0f;
	warpGrid = warpGrid.permute({ 0, 2, 3, 1 });

	return torch::nn::functional::grid_sample(imageTensor, warpGrid, torch::nn::functional::GridSampleFuncOptions().mode(torch::kBilinear).padding_mode(torch::kZeros));
}

bool PointCloudEngine::GroundTruthRenderer::LoadNeuralNetworkModel(std::wstring& filename, torch::jit::script::Module& model)
{
	try
	{
		// Load the neural network from file
		if (settings->useCUDA && torch::cuda::is_available())
		{
			model = torch::jit::load(std::string(filename.begin(), filename.end()), torch::Device(at::kCUDA));
		}
		else
		{
			model = torch::jit::load(std::string(filename.begin(), filename.end()), torch::Device(at::kCPU));
		}
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
		WARNING_MESSAGE(L"Could not load Pytorch Jit Neural Network from file " + filename + L"!\nMake sure that the engine is running in Release mode.");
		return false;
	}

#if _DEBUG
	model.dump(true, true, true);
#endif

	// Model needs to use half precision
	model.to(torch::kFloat16);

	return true;
}

torch::Tensor PointCloudEngine::GroundTruthRenderer::EvaluateNeuralNetworkModel(torch::jit::script::Module& model, torch::Tensor inputTensor)
{
	std::vector<torch::jit::IValue> inputs = { inputTensor };
	return model.forward(inputs).toTensor();
}

#ifndef min
	#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef max
	#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif