#include "PullPush.h"

PointCloudEngine::PullPush::PullPush()
{
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

		// Create the pull push texture
		ID3D11Texture2D* pullPushTexture = NULL;
		hr = d3d11Device->CreateTexture2D(&pullPushTextureDesc, NULL, &pullPushTexture);
		ERROR_MESSAGE_ON_HR(hr, NAMEOF(d3d11Device->CreateTexture2D) + L" failed!");

		// Create a shader resource view in order to read the texture in the shader (also allows texture filtering)
		D3D11_SHADER_RESOURCE_VIEW_DESC pullPushTextureSRVDesc;
		ZeroMemory(&pullPushTextureSRVDesc, sizeof(pullPushTextureSRVDesc));
		pullPushTextureSRVDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		pullPushTextureSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		pullPushTextureSRVDesc.Texture2D.MipLevels = 1;

		ID3D11ShaderResourceView* pullPushTextureSRV = NULL;
		hr = d3d11Device->CreateShaderResourceView(pullPushTexture, &pullPushTextureSRVDesc, &pullPushTextureSRV);
		ERROR_MESSAGE_ON_HR(hr, NAMEOF(d3d11Device->CreateShaderResourceView) + L" failed for the " + NAMEOF(pullPushTextureSRV));

		// Create an unordered acces views in order to access and manipulate the textures in shaders
		D3D11_UNORDERED_ACCESS_VIEW_DESC pullPushTextureUAVDesc;
		ZeroMemory(&pullPushTextureUAVDesc, sizeof(pullPushTextureUAVDesc));
		pullPushTextureUAVDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		pullPushTextureUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;

		ID3D11UnorderedAccessView* pullPushTextureUAV = NULL;
		hr = d3d11Device->CreateUnorderedAccessView(pullPushTexture, &pullPushTextureUAVDesc, &pullPushTextureUAV);
		ERROR_MESSAGE_ON_HR(hr, NAMEOF(d3d11Device->CreateUnorderedAccessView) + L" failed for the " + NAMEOF(pullPushTextureUAV));

		pullPushTextures.push_back(pullPushTexture);
		pullPushTexturesSRV.push_back(pullPushTextureSRV);
		pullPushTexturesUAV.push_back(pullPushTextureUAV);
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
	D3D11_SAMPLER_DESC pullPushSamplerDesc;
	ZeroMemory(&pullPushSamplerDesc, sizeof(pullPushSamplerDesc));
	pullPushSamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	pullPushSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	pullPushSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	pullPushSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	pullPushSamplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;

	hr = d3d11Device->CreateSamplerState(&pullPushSamplerDesc, &pullPushSamplerState);
	ERROR_MESSAGE_ON_HR(hr, NAMEOF(d3d11Device->CreateSamplerState) + L" failed for the " + NAMEOF(pullPushSamplerState));
}

void PointCloudEngine::PullPush::Execute(ID3D11UnorderedAccessView* colorUAV, ID3D11ShaderResourceView* depthSRV)
{
	// Recreate the texture hierarchy if resolution increased beyond the current hierarchy or decreased below half the resolution
	if ((max(settings->resolutionX, settings->resolutionY) > pullPushResolution) || ((2 * max(settings->resolutionX, settings->resolutionY)) < pullPushResolution))
	{
		CreatePullPushTextureHierarchy();
	}

	// Unbind backbuffer und depth textures in order to use them in the compute shader
	d3d11DevCon->OMSetRenderTargets(0, NULL, NULL);

	UINT zero = 0;
	d3d11DevCon->CSSetShader(pullPushShader->computeShader, 0, 0);
	d3d11DevCon->CSSetShaderResources(0, 1, &depthSRV);
	d3d11DevCon->CSSetUnorderedAccessViews(1, 1, &backBufferTextureUAV, &zero);
	d3d11DevCon->CSSetConstantBuffers(0, 1, &pullPushConstantBuffer);
	d3d11DevCon->CSSetSamplers(0, 1, &pullPushSamplerState);

	// Set texture resolution
	pullPushConstantBufferData.resolutionX = settings->resolutionX;
	pullPushConstantBufferData.resolutionY = settings->resolutionY;
	pullPushConstantBufferData.pullPushResolution = pullPushResolution;

	// Pull phase (go from high resolution to low resolution)
	for (int pullPushLevel = 0; pullPushLevel < pullPushLevels; pullPushLevel++)
	{
		// Update constant buffer
		pullPushConstantBufferData.isPullPhase = TRUE;
		pullPushConstantBufferData.pullPushLevel = pullPushLevel;
		d3d11DevCon->UpdateSubresource(pullPushConstantBuffer, 0, NULL, &pullPushConstantBufferData, 0, 0);

		// Set resources (for level 0, the input is the color texture that gets copied to the pull texture at level 0)
		d3d11DevCon->CSSetShaderResources(1, 1, (pullPushLevel == 0) ? nullSRV : &pullPushTexturesSRV[pullPushLevel - 1]);
		d3d11DevCon->CSSetUnorderedAccessViews(0, 1, &pullPushTexturesUAV[pullPushLevel], &zero);

		UINT threadGroupCount = ceil((pullPushResolution / pow(2, pullPushLevel) / 32.0f));
		d3d11DevCon->Dispatch(threadGroupCount, threadGroupCount, 1);

		// Unbind
		d3d11DevCon->CSSetShaderResources(1, 1, nullSRV);
		d3d11DevCon->CSSetUnorderedAccessViews(0, 1, nullUAV, &zero);
	}

	// Push phase (go from low resolution to high resolution)
	for (int pullPushLevel = pullPushLevels - 1; pullPushLevel >= 0; pullPushLevel--)
	{
		// Update constant buffer
		pullPushConstantBufferData.isPullPhase = FALSE;
		pullPushConstantBufferData.pullPushLevel = pullPushLevel;
		d3d11DevCon->UpdateSubresource(pullPushConstantBuffer, 0, NULL, &pullPushConstantBufferData, 0, 0);

		// Set resources (for level 0, the input is the push texture at level 0 that gets copied to the color texture)
		d3d11DevCon->CSSetShaderResources(1, 1, &pullPushTexturesSRV[pullPushLevel]);
		d3d11DevCon->CSSetUnorderedAccessViews(0, 1, (pullPushLevel == 0) ? nullUAV : &pullPushTexturesUAV[pullPushLevel - 1], &zero);

		UINT threadGroupCount = ceil((pullPushResolution / pow(2, max(0, pullPushLevel - 1)) / 32.0f));
		d3d11DevCon->Dispatch(threadGroupCount, threadGroupCount, 1);

		// Unbind
		d3d11DevCon->CSSetShaderResources(1, 1, nullSRV);
		d3d11DevCon->CSSetUnorderedAccessViews(0, 1, nullUAV, &zero);
	}

	// Unbind all shader resources
	d3d11DevCon->CSSetShaderResources(0, 1, nullSRV);
	d3d11DevCon->CSSetShaderResources(1, 1, nullSRV);
	d3d11DevCon->CSSetUnorderedAccessViews(0, 1, nullUAV, &zero);
	d3d11DevCon->CSSetUnorderedAccessViews(1, 1, nullUAV, &zero);

	//// DEBUG ALL TEXTURES TO FILES
	//for (int pullPushLevel = pullPushLevels - 1; pullPushLevel >= 0; pullPushLevel--)
	//{
	//	hr = SaveWICTextureToFile(d3d11DevCon, pullPushTextures[pullPushLevel], GUID_ContainerFormatPng, (executableDirectory + L"/Screenshots/PullPush" + std::to_wstring(pullPushLevel) + L".png").c_str());
	//	ERROR_MESSAGE_ON_HR(hr, NAMEOF(SaveWICTextureToFile) + L" failed in " + NAMEOF(SaveScreenshotToFile));
	//}

	//std::cout << "done" << std::endl;
}

void PointCloudEngine::PullPush::Release()
{
	SAFE_RELEASE(pullPushConstantBuffer);
	SAFE_RELEASE(pullPushSamplerState);

	for (auto it = pullPushTextures.begin(); it != pullPushTextures.end(); it++)
	{
		(*it)->Release();
	}

	for (auto it = pullPushTexturesSRV.begin(); it != pullPushTexturesSRV.end(); it++)
	{
		(*it)->Release();
	}

	for (auto it = pullPushTexturesUAV.begin(); it != pullPushTexturesUAV.end(); it++)
	{
		(*it)->Release();
	}

	pullPushTextures.clear();
	pullPushTexturesSRV.clear();
	pullPushTexturesUAV.clear();
}
