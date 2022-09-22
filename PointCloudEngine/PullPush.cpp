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
		D3D11_TEXTURE2D_DESC pullPushLevelTextureDesc;
		pullPushLevelTextureDesc.Width = pullPushResolution / pow(2, pullPushLevel);
		pullPushLevelTextureDesc.Height = pullPushLevelTextureDesc.Width;
		pullPushLevelTextureDesc.MipLevels = 1;
		pullPushLevelTextureDesc.ArraySize = 1;
		pullPushLevelTextureDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		pullPushLevelTextureDesc.SampleDesc.Count = 1;
		pullPushLevelTextureDesc.SampleDesc.Quality = 0;
		pullPushLevelTextureDesc.Usage = D3D11_USAGE_DEFAULT;
		pullPushLevelTextureDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
		pullPushLevelTextureDesc.CPUAccessFlags = 0;
		pullPushLevelTextureDesc.MiscFlags = 0;

		// Create the pull texture
		ID3D11Texture2D* pullLevelTexture = NULL;
		hr = d3d11Device->CreateTexture2D(&pullPushLevelTextureDesc, NULL, &pullLevelTexture);
		ERROR_MESSAGE_ON_HR(hr, NAMEOF(d3d11Device->CreateTexture2D) + L" failed!");

		// Create the push texture
		ID3D11Texture2D* pushLevelTexture = NULL;
		hr = d3d11Device->CreateTexture2D(&pullPushLevelTextureDesc, NULL, &pushLevelTexture);
		ERROR_MESSAGE_ON_HR(hr, NAMEOF(d3d11Device->CreateTexture2D) + L" failed!");

		// Create an unordered acces views in order to access and manipulate the textures in shaders
		D3D11_UNORDERED_ACCESS_VIEW_DESC pullPushLevelTextureUAVDesc;
		ZeroMemory(&pullPushLevelTextureUAVDesc, sizeof(pullPushLevelTextureUAVDesc));
		pullPushLevelTextureUAVDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		pullPushLevelTextureUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;

		// Create pull texture UAV
		ID3D11UnorderedAccessView* pullLevelTextureUAV = NULL;
		hr = d3d11Device->CreateUnorderedAccessView(pullLevelTexture, &pullPushLevelTextureUAVDesc, &pullLevelTextureUAV);
		ERROR_MESSAGE_ON_HR(hr, NAMEOF(d3d11Device->CreateUnorderedAccessView) + L" failed for the " + NAMEOF(pullLevelTextureUAV));

		// Create push texture UAV
		ID3D11UnorderedAccessView* pushLevelTextureUAV = NULL;
		hr = d3d11Device->CreateUnorderedAccessView(pushLevelTexture, &pullPushLevelTextureUAVDesc, &pushLevelTextureUAV);
		ERROR_MESSAGE_ON_HR(hr, NAMEOF(d3d11Device->CreateUnorderedAccessView) + L" failed for the " + NAMEOF(pushLevelTextureUAV));

		pullTextures.push_back(pullLevelTexture);
		pushTextures.push_back(pushLevelTexture);
		pullTexturesUAV.push_back(pullLevelTextureUAV);
		pushTexturesUAV.push_back(pushLevelTextureUAV);
	}
}

void PointCloudEngine::PullPush::Execute(ID3D11UnorderedAccessView* initialColorUAV, ID3D11DepthStencilView* initialDepthView)
{
	// Recreate the texture hierarchy if resolution increased beyond the current hierarchy or decreased below half the resolution
	if ((max(settings->resolutionX, settings->resolutionY) > pullPushResolution) || ((2 * max(settings->resolutionX, settings->resolutionY)) < pullPushResolution))
	{
		CreatePullPushTextureHierarchy();
	}

	// TODO: Assign RGBA values for first pull texture layer (set A to 0 if a pixel is not drawn, meaning that its depth is 1)

	// TODO: Execute pull phase

	// TODO: Execute push phase

	// TODO: Render/copy result to the backbuffer
}

void PointCloudEngine::PullPush::Release()
{
	for (auto it = pullTextures.begin(); it != pullTextures.end(); it++)
	{
		(*it)->Release();
	}

	for (auto it = pushTextures.begin(); it != pushTextures.end(); it++)
	{
		(*it)->Release();
	}

	for (auto it = pullTexturesUAV.begin(); it != pullTexturesUAV.end(); it++)
	{
		(*it)->Release();
	}

	for (auto it = pushTexturesUAV.begin(); it != pushTexturesUAV.end(); it++)
	{
		(*it)->Release();
	}

	pullTextures.clear();
	pushTextures.clear();
	pullTexturesUAV.clear();
	pushTexturesUAV.clear();
}
