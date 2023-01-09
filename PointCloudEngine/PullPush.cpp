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
		D3D11_TEXTURE2D_DESC pullPushTextureDesc;
		pullPushTextureDesc.Width = pullPushResolution / pow(2, pullPushLevel);
		pullPushTextureDesc.Height = pullPushTextureDesc.Width;
		pullPushTextureDesc.MipLevels = 1;
		pullPushTextureDesc.ArraySize = 1;
		pullPushTextureDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		pullPushTextureDesc.SampleDesc.Count = 1;
		pullPushTextureDesc.SampleDesc.Quality = 0;
		pullPushTextureDesc.Usage = D3D11_USAGE_DEFAULT;
		pullPushTextureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
		pullPushTextureDesc.CPUAccessFlags = 0;
		pullPushTextureDesc.MiscFlags = 0;

		// Create the pull push color texture
		ID3D11Texture2D* pullPushColorTexture = NULL;
		ID3D11ShaderResourceView* pullPushColorTextureSRV = NULL;
		ID3D11UnorderedAccessView* pullPushColorTextureUAV = NULL;

		CreateTextureResources(&pullPushTextureDesc, &pullPushColorTexture, &pullPushColorTextureSRV, &pullPushColorTextureUAV);

		pullPushColorTextures.push_back(pullPushColorTexture);
		pullPushColorTexturesSRV.push_back(pullPushColorTextureSRV);
		pullPushColorTexturesUAV.push_back(pullPushColorTextureUAV);

		// Create the pull push normal texture
		ID3D11Texture2D* pullPushNormalTexture = NULL;
		ID3D11ShaderResourceView* pullPushNormalTextureSRV = NULL;
		ID3D11UnorderedAccessView* pullPushNormalTextureUAV = NULL;

		CreateTextureResources(&pullPushTextureDesc, &pullPushNormalTexture, &pullPushNormalTextureSRV, &pullPushNormalTextureUAV);

		pullPushNormalTextures.push_back(pullPushNormalTexture);
		pullPushNormalTexturesSRV.push_back(pullPushNormalTextureSRV);
		pullPushNormalTexturesUAV.push_back(pullPushNormalTextureUAV);

		// Create the pull push position texture
		ID3D11Texture2D* pullPushPositionTexture = NULL;
		ID3D11ShaderResourceView* pullPushPositionTextureSRV = NULL;
		ID3D11UnorderedAccessView* pullPushPositionTextureUAV = NULL;

		CreateTextureResources(&pullPushTextureDesc, &pullPushPositionTexture, &pullPushPositionTextureSRV, &pullPushPositionTextureUAV);

		pullPushPositionTextures.push_back(pullPushPositionTexture);
		pullPushPositionTexturesSRV.push_back(pullPushPositionTextureSRV);
		pullPushPositionTexturesUAV.push_back(pullPushPositionTextureUAV);
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
	pullPushSamplerDesc.Filter = settings->pullPushLinearFilter ? D3D11_FILTER_MIN_MAG_MIP_LINEAR : D3D11_FILTER_MIN_MAG_MIP_POINT;
	pullPushSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	pullPushSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	pullPushSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	pullPushSamplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;

	hr = d3d11Device->CreateSamplerState(&pullPushSamplerDesc, &pullPushSamplerState);
	ERROR_MESSAGE_ON_HR(hr, NAMEOF(d3d11Device->CreateSamplerState) + L" failed for the " + NAMEOF(pullPushSamplerState));
}

void PointCloudEngine::PullPush::SetInitialColorTexture(ID3D11Resource* colorTexture)
{
	// Copy the backbuffer color texture that contains the rendered sparse points to the pull push texture at level 0
	d3d11DevCon->CopySubresourceRegion(pullPushColorTextures.front(), 0, 0, 0, 0, colorTexture, 0, NULL);
}

void PointCloudEngine::PullPush::SetInitialNormalTexture(ID3D11Resource* normalTexture)
{
	// Copy the normal texture that contains the rendered sparse points normals to the pull push texture at level 0
	d3d11DevCon->CopySubresourceRegion(pullPushNormalTextures.front(), 0, 0, 0, 0, normalTexture, 0, NULL);
}

void PointCloudEngine::PullPush::Execute(ID3D11Resource* backbufferTexture, ID3D11ShaderResourceView* depthSRV)
{
	// Recreate the texture hierarchy if resolution increased beyond the current hierarchy or decreased below half the resolution
	if ((max(settings->resolutionX, settings->resolutionY) > pullPushResolution) || ((2 * max(settings->resolutionX, settings->resolutionY)) < pullPushResolution))
	{
		CreatePullPushTextureHierarchy();
	}

	// Recreate the sampler state if necessary
	if ((settings->pullPushLinearFilter && (pullPushSamplerDesc.Filter == D3D11_FILTER_MIN_MAG_MIP_POINT)) || (!settings->pullPushLinearFilter && (pullPushSamplerDesc.Filter == D3D11_FILTER_MIN_MAG_MIP_LINEAR)))
	{
		SAFE_RELEASE(pullPushSamplerState);
		pullPushSamplerDesc.Filter = settings->pullPushLinearFilter ? D3D11_FILTER_MIN_MAG_MIP_LINEAR : D3D11_FILTER_MIN_MAG_MIP_POINT;

		hr = d3d11Device->CreateSamplerState(&pullPushSamplerDesc, &pullPushSamplerState);
		ERROR_MESSAGE_ON_HR(hr, NAMEOF(d3d11Device->CreateSamplerState) + L" failed for the " + NAMEOF(pullPushSamplerState));
	}

	// Unbind backbuffer und depth textures in order to use them in the compute shader
	d3d11DevCon->OMSetRenderTargets(0, NULL, NULL);

	UINT zero = 0;
	d3d11DevCon->CSSetShader(pullPushShader->computeShader, 0, 0);
	d3d11DevCon->CSSetShaderResources(0, 1, &depthSRV);
	d3d11DevCon->CSSetConstantBuffers(1, 1, &pullPushConstantBuffer);
	d3d11DevCon->CSSetSamplers(0, 1, &pullPushSamplerState);

	// Set texture resolution
	pullPushConstantBufferData.orientedSplat = settings->pullPushOrientedSplat;
	pullPushConstantBufferData.texelBlending = settings->pullPushBlending;
	pullPushConstantBufferData.resolutionPullPush = pullPushResolution;
	pullPushConstantBufferData.blendRange = settings->pullPushBlendRange;
	pullPushConstantBufferData.splatSize = settings->pullPushSplatSize;
	pullPushConstantBufferData.nearZ = settings->nearZ;
	pullPushConstantBufferData.farZ = settings->farZ;

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
		d3d11DevCon->CSSetShaderResources(2, 1, (pullPushLevel == 0) ? nullSRV : &pullPushNormalTexturesSRV[pullPushLevel - 1]);
		d3d11DevCon->CSSetShaderResources(3, 1, (pullPushLevel == 0) ? nullSRV : &pullPushPositionTexturesSRV[pullPushLevel - 1]);
		d3d11DevCon->CSSetUnorderedAccessViews(0, 1, &pullPushColorTexturesUAV[pullPushLevel], &zero);
		d3d11DevCon->CSSetUnorderedAccessViews(1, 1, &pullPushNormalTexturesUAV[pullPushLevel], &zero);
		d3d11DevCon->CSSetUnorderedAccessViews(2, 1, &pullPushPositionTexturesUAV[pullPushLevel], &zero);

		UINT threadGroupCount = ceil(pullPushConstantBufferData.resolutionOutput / 32.0f);
		d3d11DevCon->Dispatch(threadGroupCount, threadGroupCount, 1);

		// Unbind
		d3d11DevCon->CSSetShaderResources(1, 1, nullSRV);
		d3d11DevCon->CSSetShaderResources(2, 1, nullSRV);
		d3d11DevCon->CSSetShaderResources(3, 1, nullSRV);
		d3d11DevCon->CSSetUnorderedAccessViews(0, 1, nullUAV, &zero);
		d3d11DevCon->CSSetUnorderedAccessViews(1, 1, nullUAV, &zero);
		d3d11DevCon->CSSetUnorderedAccessViews(2, 1, nullUAV, &zero);
	}

	// Possibly debug the pull phase textures
	if (settings->pullPushSkipPushPhase && (settings->pullPushDebugLevel > 0))
	{
		pullPushConstantBufferData.debug = true;
		pullPushConstantBufferData.resolutionOutput = pullPushResolution;
		d3d11DevCon->UpdateSubresource(pullPushConstantBuffer, 0, NULL, &pullPushConstantBufferData, 0, 0);

		// Draw the texture at the debug level directly to the output at level 0
		d3d11DevCon->CSSetShaderResources(1, 1, &pullPushColorTexturesSRV[min(settings->pullPushDebugLevel, pullPushLevels - 1)]);
		d3d11DevCon->CSSetUnorderedAccessViews(0, 1, &pullPushColorTexturesUAV[0], &zero);

		UINT threadGroupCount = ceil(pullPushResolution / 32.0f);
		d3d11DevCon->Dispatch(threadGroupCount, threadGroupCount, 1);

		// Unbind
		d3d11DevCon->CSSetShaderResources(1, 1, nullSRV);
		d3d11DevCon->CSSetUnorderedAccessViews(0, 1, nullUAV, &zero);

		pullPushConstantBufferData.debug = false;
	}

	if (Input::GetKeyDown(DirectX::Keyboard::K))
	{
		// DEBUG ALL TEXTURES TO FILES
		for (int pullPushLevel = pullPushLevels - 1; pullPushLevel >= 0; pullPushLevel--)
		{
			hr = SaveWICTextureToFile(d3d11DevCon, pullPushColorTextures[pullPushLevel], GUID_ContainerFormatPng, (executableDirectory + L"/Screenshots/Pull" + std::to_wstring(pullPushLevel) + L".png").c_str());
			ERROR_MESSAGE_ON_HR(hr, NAMEOF(SaveWICTextureToFile) + L" failed in " + NAMEOF(SaveScreenshotToFile));
		}

		std::cout << "done" << std::endl;
	}

	if (!settings->pullPushSkipPushPhase)
	{
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
			d3d11DevCon->CSSetShaderResources(2, 1, &pullPushNormalTexturesSRV[pullPushLevel]);
			d3d11DevCon->CSSetShaderResources(3, 1, &pullPushPositionTexturesSRV[pullPushLevel]);
			d3d11DevCon->CSSetUnorderedAccessViews(0, 1, &pullPushColorTexturesUAV[pullPushLevel - 1], &zero);
			d3d11DevCon->CSSetUnorderedAccessViews(1, 1, &pullPushNormalTexturesUAV[pullPushLevel - 1], &zero);
			d3d11DevCon->CSSetUnorderedAccessViews(2, 1, &pullPushPositionTexturesUAV[pullPushLevel - 1], &zero);

			UINT threadGroupCount = ceil(pullPushConstantBufferData.resolutionOutput / 32.0f);
			d3d11DevCon->Dispatch(threadGroupCount, threadGroupCount, 1);

			// Unbind
			d3d11DevCon->CSSetShaderResources(1, 1, nullSRV);
			d3d11DevCon->CSSetShaderResources(2, 1, nullSRV);
			d3d11DevCon->CSSetShaderResources(3, 1, nullSRV);
			d3d11DevCon->CSSetUnorderedAccessViews(0, 1, nullUAV, &zero);
			d3d11DevCon->CSSetUnorderedAccessViews(1, 1, nullUAV, &zero);
			d3d11DevCon->CSSetUnorderedAccessViews(2, 1, nullUAV, &zero);
		}

		// Possibly debug the push phase textures
		if (settings->pullPushDebugLevel > 0)
		{
			pullPushConstantBufferData.debug = true;
			pullPushConstantBufferData.resolutionOutput = pullPushResolution;
			d3d11DevCon->UpdateSubresource(pullPushConstantBuffer, 0, NULL, &pullPushConstantBufferData, 0, 0);

			// Draw the texture at the debug level directly to the output at level 0
			d3d11DevCon->CSSetShaderResources(1, 1, &pullPushColorTexturesSRV[min(settings->pullPushDebugLevel, pullPushLevels - 1)]);
			d3d11DevCon->CSSetUnorderedAccessViews(0, 1, &pullPushColorTexturesUAV[0], &zero);

			UINT threadGroupCount = ceil(pullPushResolution / 32.0f);
			d3d11DevCon->Dispatch(threadGroupCount, threadGroupCount, 1);

			// Unbind
			d3d11DevCon->CSSetShaderResources(1, 1, nullSRV);
			d3d11DevCon->CSSetUnorderedAccessViews(0, 1, nullUAV, &zero);

			pullPushConstantBufferData.debug = false;
		}
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

	d3d11DevCon->CopySubresourceRegion(backbufferTexture, 0, 0, 0, 0, pullPushColorTextures.front(), 0, &pullPushBox);

	if (Input::GetKeyDown(DirectX::Keyboard::K))
	{
		// DEBUG ALL TEXTURES TO FILES
		for (int pullPushLevel = pullPushLevels - 1; pullPushLevel >= 0; pullPushLevel--)
		{
			hr = SaveWICTextureToFile(d3d11DevCon, pullPushColorTextures[pullPushLevel], GUID_ContainerFormatPng, (executableDirectory + L"/Screenshots/Push" + std::to_wstring(pullPushLevel) + L".png").c_str());
			ERROR_MESSAGE_ON_HR(hr, NAMEOF(SaveWICTextureToFile) + L" failed in " + NAMEOF(SaveScreenshotToFile));
		}

		std::cout << "done" << std::endl;
	}
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

		SAFE_RELEASE(pullPushNormalTextures[pullPushLevel]);
		SAFE_RELEASE(pullPushNormalTexturesSRV[pullPushLevel]);
		SAFE_RELEASE(pullPushNormalTexturesUAV[pullPushLevel]);

		SAFE_RELEASE(pullPushPositionTextures[pullPushLevel]);
		SAFE_RELEASE(pullPushPositionTexturesSRV[pullPushLevel]);
		SAFE_RELEASE(pullPushPositionTexturesUAV[pullPushLevel]);
	}

	pullPushColorTextures.clear();
	pullPushColorTexturesSRV.clear();
	pullPushColorTexturesUAV.clear();

	pullPushNormalTextures.clear();
	pullPushNormalTexturesSRV.clear();
	pullPushNormalTexturesUAV.clear();

	pullPushPositionTextures.clear();
	pullPushPositionTexturesSRV.clear();
	pullPushPositionTexturesUAV.clear();

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
