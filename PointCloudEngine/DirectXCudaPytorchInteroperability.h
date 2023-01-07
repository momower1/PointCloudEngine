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
	static void Execute();

private:
	static bool initialized;
	static CUresult cudaResult;
	static CUdevice cudaDevice;
	static CUcontext cudaContext;
	static CUstream cudaStream;
	static CUgraphicsResource cudaGraphicsResourceBackbuffer;
};

#endif
