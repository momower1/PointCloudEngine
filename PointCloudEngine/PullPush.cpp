#include "PullPush.h"

PointCloudEngine::PullPush::PullPush()
{
	pullPushLevels = 0;
	CreatePullPushTextureHierarchy();
}

void PointCloudEngine::PullPush::CreatePullPushTextureHierarchy()
{
	Release();

	// For simplicity use power of two resolution for the pull push algorithm (not ideal e.g. if intitial resolution is 511x1, then pull push uses 1024x1024)
	pullPushResolution = pow(2, ceil(log2(max(settings->resolutionX, settings->resolutionY))));
	pullPushLevels = log2(pullPushResolution) + 1;

	// Create all the required textures (level 0 has full resolution, last level has 1x1 resolution)
	for (int pullPushLevel = 0; pullPushLevel < pullPushLevels; pullPushLevel++)
	{
		// Use a 16-bit floating point texture and store the weights in the alpha channel)
		D3D11_TEXTURE2D_DESC pullPushColorTextureDesc;
		pullPushColorTextureDesc.Width = pullPushResolution / pow(2, pullPushLevel);
		pullPushColorTextureDesc.Height = pullPushColorTextureDesc.Width;
		pullPushColorTextureDesc.MipLevels = 1;
		pullPushColorTextureDesc.ArraySize = 1;
		pullPushColorTextureDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		pullPushColorTextureDesc.SampleDesc.Count = 1;
		pullPushColorTextureDesc.SampleDesc.Quality = 0;
		pullPushColorTextureDesc.Usage = D3D11_USAGE_DEFAULT;
		pullPushColorTextureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
		pullPushColorTextureDesc.CPUAccessFlags = 0;
		pullPushColorTextureDesc.MiscFlags = 0;

		// Create the pull push color texture
		ID3D11Texture2D* pullPushColorTexture = NULL;
		ID3D11ShaderResourceView* pullPushColorTextureSRV = NULL;
		ID3D11UnorderedAccessView* pullPushColorTextureUAV = NULL;

		CreateTextureResources(&pullPushColorTextureDesc, &pullPushColorTexture, &pullPushColorTextureSRV, &pullPushColorTextureUAV);

		pullPushColorTextures.push_back(pullPushColorTexture);
		pullPushColorTexturesSRV.push_back(pullPushColorTextureSRV);
		pullPushColorTexturesUAV.push_back(pullPushColorTextureUAV);

		// Create the pull push importance texture
		D3D11_TEXTURE2D_DESC pullPushImportanceTextureDesc = pullPushColorTextureDesc;
		pullPushImportanceTextureDesc.Format = DXGI_FORMAT_R16_FLOAT;

		ID3D11Texture2D* pullPushImportanceTexture = NULL;
		ID3D11ShaderResourceView* pullPushImportanceTextureSRV = NULL;
		ID3D11UnorderedAccessView* pullPushImportanceTextureUAV = NULL;

		CreateTextureResources(&pullPushImportanceTextureDesc, &pullPushImportanceTexture, &pullPushImportanceTextureSRV, &pullPushImportanceTextureUAV);

		pullPushImportanceTextures.push_back(pullPushImportanceTexture);
		pullPushImportanceTexturesSRV.push_back(pullPushImportanceTextureSRV);
		pullPushImportanceTexturesUAV.push_back(pullPushImportanceTextureUAV);
	}

	// Create the constant buffer for the shader
	D3D11_BUFFER_DESC pullPushConstantBufferDesc;
	ZeroMemory(&pullPushConstantBufferDesc, sizeof(pullPushConstantBufferDesc));
	pullPushConstantBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	pullPushConstantBufferDesc.ByteWidth = sizeof(pullPushConstantBufferData);
	pullPushConstantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	pullPushConstantBufferDesc.CPUAccessFlags = 0;
	pullPushConstantBufferDesc.MiscFlags = 0;

	hr = d3d11Device->CreateBuffer(&pullPushConstantBufferDesc, NULL, &pullPushConstantBuffer);
	ERROR_MESSAGE_ON_HR(hr, NAMEOF(d3d11Device->CreateBuffer) + L" failed for the " + NAMEOF(pullPushConstantBuffer));

	// Create texture sampler
	ZeroMemory(&pullPushSamplerDesc, sizeof(pullPushSamplerDesc));
	pullPushSamplerDesc.Filter = settings->usePullPushLinearFilter ? D3D11_FILTER_MIN_MAG_MIP_LINEAR : D3D11_FILTER_MIN_MAG_MIP_POINT;
	pullPushSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	pullPushSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	pullPushSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	pullPushSamplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;

	hr = d3d11Device->CreateSamplerState(&pullPushSamplerDesc, &pullPushSamplerState);
	ERROR_MESSAGE_ON_HR(hr, NAMEOF(d3d11Device->CreateSamplerState) + L" failed for the " + NAMEOF(pullPushSamplerState));
}

void PointCloudEngine::PullPush::Execute(ID3D11Resource* colorTexture, ID3D11ShaderResourceView* depthSRV)
{
	// Recreate the texture hierarchy if resolution increased beyond the current hierarchy or decreased below half the resolution
	if ((max(settings->resolutionX, settings->resolutionY) > pullPushResolution) || ((2 * max(settings->resolutionX, settings->resolutionY)) < pullPushResolution))
	{
		CreatePullPushTextureHierarchy();
	}

	// Recreate the sampler state if necessary
	if ((settings->usePullPushLinearFilter && (pullPushSamplerDesc.Filter == D3D11_FILTER_MIN_MAG_MIP_POINT)) || (!settings->usePullPushLinearFilter && (pullPushSamplerDesc.Filter == D3D11_FILTER_MIN_MAG_MIP_LINEAR)))
	{
		SAFE_RELEASE(pullPushSamplerState);
		pullPushSamplerDesc.Filter = settings->usePullPushLinearFilter ? D3D11_FILTER_MIN_MAG_MIP_LINEAR : D3D11_FILTER_MIN_MAG_MIP_POINT;

		hr = d3d11Device->CreateSamplerState(&pullPushSamplerDesc, &pullPushSamplerState);
		ERROR_MESSAGE_ON_HR(hr, NAMEOF(d3d11Device->CreateSamplerState) + L" failed for the " + NAMEOF(pullPushSamplerState));
	}

	// Unbind backbuffer und depth textures in order to use them in the compute shader
	d3d11DevCon->OMSetRenderTargets(0, NULL, NULL);

	// Copy the backbuffer color texture that contains the rendered sparse points to the pull push texture at level 0
	d3d11DevCon->CopySubresourceRegion(pullPushColorTextures.front(), 0, 0, 0, 0, colorTexture, 0, NULL);

	UINT zero = 0;
	d3d11DevCon->CSSetShader(pullPushShader->computeShader, 0, 0);
	d3d11DevCon->CSSetShaderResources(0, 1, &depthSRV);
	d3d11DevCon->CSSetConstantBuffers(0, 1, &pullPushConstantBuffer);
	d3d11DevCon->CSSetSamplers(0, 1, &pullPushSamplerState);

	// Set texture resolution
	pullPushConstantBufferData.resolutionX = settings->resolutionX;
	pullPushConstantBufferData.resolutionY = settings->resolutionY;

	// Pull phase (go from high resolution to low resolution)
	for (int pullPushLevel = 0; pullPushLevel < pullPushLevels; pullPushLevel++)
	{
		// Update constant buffer
		pullPushConstantBufferData.isPullPhase = TRUE;
		pullPushConstantBufferData.pullPushLevel = pullPushLevel;
		pullPushConstantBufferData.resolutionOutput = pullPushResolution / pow(2, pullPushLevel);
		d3d11DevCon->UpdateSubresource(pullPushConstantBuffer, 0, NULL, &pullPushConstantBufferData, 0, 0);

		// Set resources (for level 0, only the output texture is required)
		d3d11DevCon->CSSetShaderResources(1, 1, (pullPushLevel == 0) ? nullSRV : &pullPushColorTexturesSRV[pullPushLevel - 1]);
		d3d11DevCon->CSSetShaderResources(2, 1, (pullPushLevel == 0) ? nullSRV : &pullPushImportanceTexturesSRV[pullPushLevel - 1]);
		d3d11DevCon->CSSetUnorderedAccessViews(0, 1, &pullPushColorTexturesUAV[pullPushLevel], &zero);
		d3d11DevCon->CSSetUnorderedAccessViews(1, 1, &pullPushImportanceTexturesUAV[pullPushLevel], &zero);

		UINT threadGroupCount = ceil(pullPushConstantBufferData.resolutionOutput / 32.0f);
		d3d11DevCon->Dispatch(threadGroupCount, threadGroupCount, 1);

		// Unbind
		d3d11DevCon->CSSetShaderResources(1, 1, nullSRV);
		d3d11DevCon->CSSetShaderResources(2, 1, nullSRV);
		d3d11DevCon->CSSetUnorderedAccessViews(0, 1, nullUAV, &zero);
		d3d11DevCon->CSSetUnorderedAccessViews(1, 1, nullUAV, &zero);
	}

	// Push phase (go from low resolution to high resolution)
	for (int pullPushLevel = pullPushLevels - 1; pullPushLevel > 0; pullPushLevel--)
	{
		// Update constant buffer
		pullPushConstantBufferData.isPullPhase = FALSE;
		pullPushConstantBufferData.pullPushLevel = pullPushLevel;
		pullPushConstantBufferData.resolutionOutput = pullPushResolution / pow(2, max(0, pullPushLevel - 1));
		d3d11DevCon->UpdateSubresource(pullPushConstantBuffer, 0, NULL, &pullPushConstantBufferData, 0, 0);

		// Set resources
		d3d11DevCon->CSSetShaderResources(1, 1, &pullPushColorTexturesSRV[pullPushLevel]);
		d3d11DevCon->CSSetShaderResources(2, 1, &pullPushImportanceTexturesSRV[pullPushLevel]);
		d3d11DevCon->CSSetUnorderedAccessViews(0, 1, &pullPushColorTexturesUAV[pullPushLevel - 1], &zero);
		d3d11DevCon->CSSetUnorderedAccessViews(1, 1, &pullPushImportanceTexturesUAV[pullPushLevel - 1], &zero);

		UINT threadGroupCount = ceil(pullPushConstantBufferData.resolutionOutput / 32.0f);
		d3d11DevCon->Dispatch(threadGroupCount, threadGroupCount, 1);

		// Unbind
		d3d11DevCon->CSSetShaderResources(1, 1, nullSRV);
		d3d11DevCon->CSSetShaderResources(2, 1, nullSRV);
		d3d11DevCon->CSSetUnorderedAccessViews(0, 1, nullUAV, &zero);
		d3d11DevCon->CSSetUnorderedAccessViews(1, 1, nullUAV, &zero);
	}

	// Unbind remaining shader resources
	d3d11DevCon->CSSetShaderResources(0, 1, nullSRV);

	// Copy the result from the pull push algorithm back to the backbuffer color texture
	D3D11_BOX pullPushBox;
	pullPushBox.left = 0;
	pullPushBox.right = settings->resolutionX;
	pullPushBox.top = 0;
	pullPushBox.bottom = settings->resolutionY;
	pullPushBox.front = 0;
	pullPushBox.back = 1;

	d3d11DevCon->CopySubresourceRegion(colorTexture, 0, 0, 0, 0, pullPushColorTextures.front(), 0, &pullPushBox);

	//// DEBUG ALL TEXTURES TO FILES
	//for (int pullPushLevel = pullPushLevels - 1; pullPushLevel >= 0; pullPushLevel--)
	//{
	//	hr = SaveWICTextureToFile(d3d11DevCon, pullPushColorTextures[pullPushLevel], GUID_ContainerFormatPng, (executableDirectory + L"/Screenshots/PullPush" + std::to_wstring(pullPushLevel) + L".png").c_str());
	//	ERROR_MESSAGE_ON_HR(hr, NAMEOF(SaveWICTextureToFile) + L" failed in " + NAMEOF(SaveScreenshotToFile));
	//}

	//std::cout << "done" << std::endl;
}

void PointCloudEngine::PullPush::Release()
{
	SAFE_RELEASE(pullPushConstantBuffer);
	SAFE_RELEASE(pullPushSamplerState);

	for (int pullPushLevel = 0; pullPushLevel < pullPushLevels; pullPushLevel++)
	{
		SAFE_RELEASE(pullPushColorTextures[pullPushLevel]);
		SAFE_RELEASE(pullPushColorTexturesSRV[pullPushLevel]);
		SAFE_RELEASE(pullPushColorTexturesUAV[pullPushLevel]);

		SAFE_RELEASE(pullPushImportanceTextures[pullPushLevel]);
		SAFE_RELEASE(pullPushImportanceTexturesSRV[pullPushLevel]);
		SAFE_RELEASE(pullPushImportanceTexturesUAV[pullPushLevel]);
	}

	pullPushColorTextures.clear();
	pullPushColorTexturesSRV.clear();
	pullPushColorTexturesUAV.clear();

	pullPushImportanceTextures.clear();
	pullPushImportanceTexturesSRV.clear();
	pullPushImportanceTexturesUAV.clear();

	pullPushLevels = 0;
}

void PointCloudEngine::PullPush::CreateTextureResources(D3D11_TEXTURE2D_DESC* textureDesc, ID3D11Texture2D** outTexture, ID3D11ShaderResourceView** outTextureSRV, ID3D11UnorderedAccessView** outTextureUAV)
{
	// Create the texture
	hr = d3d11Device->CreateTexture2D(textureDesc, NULL, outTexture);
	ERROR_MESSAGE_ON_HR(hr, NAMEOF(d3d11Device->CreateTexture2D) + L" failed!");

	// Create a shader resource view in order to read the texture in shaders (also allows texture filtering)
	D3D11_SHADER_RESOURCE_VIEW_DESC textureSRVDesc;
	ZeroMemory(&textureSRVDesc, sizeof(textureSRVDesc));
	textureSRVDesc.Format = textureDesc->Format;
	textureSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	textureSRVDesc.Texture2D.MipLevels = 1;

	hr = d3d11Device->CreateShaderResourceView(*outTexture, &textureSRVDesc, outTextureSRV);
	ERROR_MESSAGE_ON_HR(hr, NAMEOF(d3d11Device->CreateShaderResourceView) + L" failed!");

	// Create an unordered acces views in order to access and manipulate the texture in shaders
	D3D11_UNORDERED_ACCESS_VIEW_DESC textureUAVDesc;
	ZeroMemory(&textureUAVDesc, sizeof(textureUAVDesc));
	textureUAVDesc.Format = textureDesc->Format;
	textureUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;

	hr = d3d11Device->CreateUnorderedAccessView(*outTexture, &textureUAVDesc, outTextureUAV);
	ERROR_MESSAGE_ON_HR(hr, NAMEOF(d3d11Device->CreateUnorderedAccessView) + L" failed!");
}
