#ifndef DIRECTXCUDAPYTORCHINTEROPERABILITY_H
#define DIRECTXCUDAPYTORCHINTEROPERABILITY_H

#include "PointCloudEngine.h"

#define DXCUDATORCH DirectXCudaPytorchInteroperability

class DirectXCudaPytorchInteroperability
{
public:
	static void Initialize();
	static void Release();
	static void InitializeSharedResources();
	static void ReleaseSharedResources();
	static torch::Tensor GetDepthTensor();
	static torch::Tensor GetBackbufferTensor();
	static void SetBackbufferFromTensor(torch::Tensor& tensor);

private:
	static bool initialized;
	static CUresult cudaResult;
	static CUdevice cudaDevice;
	static CUcontext cudaContext;
	static CUstream cudaStream;
	static ID3D11Texture2D* depthTextureCopy;
	static CUgraphicsResource cudaGraphicsResourceDepth;
	static CUgraphicsResource cudaGraphicsResourceBackbuffer;

	static torch::Tensor GetTensorFromSharedTexture(CUgraphicsResource &cudaGraphicsResource);
	static void SetSharedTextureFromTensor(CUgraphicsResource &cudaGraphicsResource, torch::Tensor &tensor);
};

#endif
