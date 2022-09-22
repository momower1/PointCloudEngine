#include "PullPush.h"

PointCloudEngine::PullPush::PullPush(int initialResolutionX, int initialResolutionY)
{
}

void PointCloudEngine::PullPush::Execute(ID3D11UnorderedAccessView* initialColorUAV, ID3D11DepthStencilView* initialDepthView)
{
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

	pullTextures.clear();
	pushTextures.clear();
}
