#include "PrecompiledHeader.h"
#include "DirectXCudaPytorchInteroperability.h"

bool DirectXCudaPytorchInteroperability::initialized = false;
CUresult DirectXCudaPytorchInteroperability::cudaResult;
CUdevice DirectXCudaPytorchInteroperability::cudaDevice;
CUcontext DirectXCudaPytorchInteroperability::cudaContext = NULL;
CUstream DirectXCudaPytorchInteroperability::cudaStream = NULL;
ID3D11Texture2D* DirectXCudaPytorchInteroperability::depthTextureCopy = NULL;
CUgraphicsResource DirectXCudaPytorchInteroperability::cudaGraphicsResourceDepth = NULL;
CUgraphicsResource DirectXCudaPytorchInteroperability::cudaGraphicsResourceBackbuffer = NULL;

void DirectXCudaPytorchInteroperability::Initialize()
{
	if (initialized)
	{
		return;
	}

	// Need to explicitly load Pytorch cuda dll in order for CUDA to be found by Pytorch
	LoadLibrary(L"torch_cuda.dll");
	//LoadLibrary(L"c10_cuda.dll");
	//LoadLibrary(L"torch_cuda_cpp.dll");
	//LoadLibrary(L"torch_cuda_cu.dll");

	ERROR_MESSAGE_ON_FAIL(torch::cuda::is_available(), L"Pytorch: CUDA not available!");

	// Initialize the CUDA library
	cudaResult = cuInit(0);
	ERROR_MESSAGE_ON_CUDA(cudaResult, NAMEOF(cuInit) + L" failed!");

	// Get the CUDA device that matches the DirectX device
	UINT cudaDeviceCount;
	cudaResult = cuD3D11GetDevices(&cudaDeviceCount, &cudaDevice, 1, d3d11Device, CUd3d11DeviceList::CU_D3D11_DEVICE_LIST_ALL);
	ERROR_MESSAGE_ON_CUDA(cudaResult, NAMEOF(cuD3D11GetDevices) + L" failed!");

	// Create a CUDA device context
	cudaResult = cuCtxCreate(&cudaContext, CU_CTX_SCHED_AUTO, cudaDevice);
	ERROR_MESSAGE_ON_CUDA(cudaResult, NAMEOF(cuCtxCreate) + L" failed!");

	// Create a CUDA stream that is required for memory mapping
	cudaResult = cuStreamCreate(&cudaStream, CU_STREAM_DEFAULT);
	ERROR_MESSAGE_ON_CUDA(cudaResult, NAMEOF(cuStreamCreate) + L" failed!");

	initialized = true;
}

void DirectXCudaPytorchInteroperability::Release()
{
	ReleaseSharedResources();

	if (cudaStream != NULL)
	{
		cudaResult = cuStreamDestroy(cudaStream);
		ERROR_MESSAGE_ON_CUDA(cudaResult, NAMEOF(cuStreamDestroy) + L" failed!");
		cudaStream = NULL;
	}

	if (cudaContext != NULL)
	{
		cudaResult = cuCtxDestroy(cudaContext);
		ERROR_MESSAGE_ON_CUDA(cudaResult, NAMEOF(cuCtxDestroy) + L" failed!");
		cudaContext = NULL;
	}

	initialized = false;
}

void DirectXCudaPytorchInteroperability::InitializeSharedResources()
{
	ReleaseSharedResources();

	// Need to create a copy of the depth stencil texture in order to share it with CUDA
	if (depthTextureCopy == NULL)
	{
		D3D11_TEXTURE2D_DESC depthTextureCopyDesc;
		depthStencilTexture->GetDesc(&depthTextureCopyDesc);

		// Convert from DXGI_FORMAT_D32_FLOAT to DXGI_FORMAT_R32_FLOAT
		depthTextureCopyDesc.Format = DXGI_FORMAT_R32_FLOAT;
		depthTextureCopyDesc.Usage = D3D11_USAGE_DEFAULT;
		depthTextureCopyDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

		hr = d3d11Device->CreateTexture2D(&depthTextureCopyDesc, NULL, &depthTextureCopy);
		ERROR_MESSAGE_ON_HR(hr, NAMEOF(d3d11Device->CreateTexture2D) + L" failed!");
	}

	if (cudaGraphicsResourceDepth == NULL)
	{
		cudaResult = cuGraphicsD3D11RegisterResource(&cudaGraphicsResourceDepth, depthTextureCopy, CU_GRAPHICS_REGISTER_FLAGS_NONE);
		ERROR_MESSAGE_ON_CUDA(cudaResult, NAMEOF(cuGraphicsD3D11RegisterResource) + L" failed!");
	}

	// Register a shared handle to the DirectX backbuffer texture
	if (cudaGraphicsResourceBackbuffer == NULL)
	{
		cudaResult = cuGraphicsD3D11RegisterResource(&cudaGraphicsResourceBackbuffer, backBufferTexture, CU_GRAPHICS_REGISTER_FLAGS_NONE);
		ERROR_MESSAGE_ON_CUDA(cudaResult, NAMEOF(cuGraphicsD3D11RegisterResource) + L" failed!");
	}
}

void DirectXCudaPytorchInteroperability::ReleaseSharedResources()
{
	SAFE_RELEASE(depthTextureCopy);

	if (cudaGraphicsResourceDepth != NULL)
	{
		cudaResult = cuGraphicsUnregisterResource(cudaGraphicsResourceDepth);
		ERROR_MESSAGE_ON_CUDA(cudaResult, NAMEOF(cuGraphicsUnregisterResource) + L" failed!");
		cudaGraphicsResourceDepth = NULL;
	}

	if (cudaGraphicsResourceBackbuffer != NULL)
	{
		cudaResult = cuGraphicsUnregisterResource(cudaGraphicsResourceBackbuffer);
		ERROR_MESSAGE_ON_CUDA(cudaResult, NAMEOF(cuGraphicsUnregisterResource) + L" failed!");
		cudaGraphicsResourceBackbuffer = NULL;
	}
}

torch::Tensor DirectXCudaPytorchInteroperability::GetDepthTensor()
{
	return torch::Tensor();
}

void DirectXCudaPytorchInteroperability::Execute()
{
	at::Tensor tensor = GetTensorFromSharedTexture(cudaGraphicsResourceBackbuffer);

	long long n = tensor.size(0);
	long long h = tensor.size(1);
	long long w = tensor.size(2);
	long long c = tensor.size(3);

	// Perform operations on the tensor
	tensor.index_put_({ at::indexing::Slice(), at::indexing::Slice(), at::indexing::Slice(0, w / 2), at::indexing::Slice() }, 0.0f);
	tensor.index_put_({ at::indexing::Slice(), at::indexing::Slice(0, h / 3), at::indexing::Slice(0, w / 2), at::indexing::Slice(0, 1) }, 1.0f);
	tensor.index_put_({ at::indexing::Slice(), at::indexing::Slice(h / 3, (2 * h) / 3), at::indexing::Slice(0, w / 2), at::indexing::Slice(1, 2) }, 1.0f);
	tensor.index_put_({ at::indexing::Slice(), at::indexing::Slice((2 * h) / 3, h), at::indexing::Slice(0, w / 2), at::indexing::Slice(2, 3) }, 1.0f);
	//tensor.index_put_({ at::indexing::Slice(), at::indexing::Slice(), at::indexing::Slice(w / 2, w), at::indexing::Slice() }, 1.0f);

	SetSharedTextureFromTensor(cudaGraphicsResourceBackbuffer, tensor);
}

void DirectXCudaPytorchInteroperability::SetSharedTextureFromTensor(CUgraphicsResource &cudaGraphicsResource, torch::Tensor &tensor)
{
	cudaResult = cuGraphicsMapResources(1, &cudaGraphicsResource, cudaStream);
	ERROR_MESSAGE_ON_CUDA(cudaResult, NAMEOF(cuGraphicsMapResources) + L" failed!");

	// Map the texture graphics resource to an array
	CUarray cudaArray;
	cudaResult = cuGraphicsSubResourceGetMappedArray(&cudaArray, cudaGraphicsResource, 0, 0);
	ERROR_MESSAGE_ON_CUDA(cudaResult, NAMEOF(cuGraphicsSubResourceGetMappedArray) + L" failed!");

	CUDA_ARRAY_DESCRIPTOR cudaArrayDescriptor;
	cudaResult = cuArrayGetDescriptor(&cudaArrayDescriptor, cudaArray);

	long long elementSizeInBytes;
	torch::ScalarType scalarType;

	if (cudaArrayDescriptor.Format == CU_AD_FORMAT_FLOAT)
	{
		elementSizeInBytes = 4;
		scalarType = torch::kFloat32;
	}
	else if (cudaArrayDescriptor.Format == CU_AD_FORMAT_HALF)
	{
		elementSizeInBytes = 2;
		scalarType = torch::kFloat16;
	}
	else
	{
		ERROR_MESSAGE(L"CUDA array format not supported.");
	}

	long long c = cudaArrayDescriptor.NumChannels;
	long long h = cudaArrayDescriptor.Height;
	long long w = cudaArrayDescriptor.Width;

	// Copy from tensor into CUDA array
	CUDA_MEMCPY2D memCopy2D;
	ZeroMemory(&memCopy2D, sizeof(memCopy2D));
	memCopy2D.srcMemoryType = CU_MEMORYTYPE_DEVICE;
	memCopy2D.dstMemoryType = CU_MEMORYTYPE_ARRAY;
	memCopy2D.dstArray = cudaArray;
	memCopy2D.srcDevice = (CUdeviceptr)tensor.data_ptr();
	memCopy2D.dstPitch = elementSizeInBytes * c * w;
	memCopy2D.srcXInBytes = 0;
	memCopy2D.srcY = 0;
	memCopy2D.dstXInBytes = 0;
	memCopy2D.dstY = 0;
	memCopy2D.WidthInBytes = elementSizeInBytes * c * w;
	memCopy2D.Height = h;

	cudaResult = cuMemcpy2D(&memCopy2D);
	ERROR_MESSAGE_ON_CUDA(cudaResult, NAMEOF(cuMemcpy2D) + L" failed!");

	cudaResult = cuGraphicsUnmapResources(1, &cudaGraphicsResource, cudaStream);
	ERROR_MESSAGE_ON_CUDA(cudaResult, NAMEOF(cuGraphicsUnmapResources) + L" failed!");
}

torch::Tensor DirectXCudaPytorchInteroperability::GetTensorFromSharedTexture(CUgraphicsResource &cudaGraphicsResource)
{
	cudaResult = cuGraphicsMapResources(1, &cudaGraphicsResource, cudaStream);
	ERROR_MESSAGE_ON_CUDA(cudaResult, NAMEOF(cuGraphicsMapResources) + L" failed!");

	// Map the texture graphics resource to an array
	CUarray cudaArray;
	cudaResult = cuGraphicsSubResourceGetMappedArray(&cudaArray, cudaGraphicsResource, 0, 0);
	ERROR_MESSAGE_ON_CUDA(cudaResult, NAMEOF(cuGraphicsSubResourceGetMappedArray) + L" failed!");

	CUDA_ARRAY_DESCRIPTOR cudaArrayDescriptor;
	cudaResult = cuArrayGetDescriptor(&cudaArrayDescriptor, cudaArray);

	long long elementSizeInBytes;
	torch::ScalarType scalarType;

	if (cudaArrayDescriptor.Format == CU_AD_FORMAT_FLOAT)
	{
		elementSizeInBytes = 4;
		scalarType = torch::kFloat32;
	}
	else if (cudaArrayDescriptor.Format == CU_AD_FORMAT_HALF)
	{
		elementSizeInBytes = 2;
		scalarType = torch::kFloat16;
	}
	else
	{
		ERROR_MESSAGE(L"CUDA array format not supported.");
	}

	long long c = cudaArrayDescriptor.NumChannels;
	long long h = cudaArrayDescriptor.Height;
	long long w = cudaArrayDescriptor.Width;

	// Need to allocate and copy to a cuda device pointer that can be used in Pytorch
	CUdeviceptr cudaData;
	cudaResult = cuMemAlloc(&cudaData, elementSizeInBytes * c * h * w);
	ERROR_MESSAGE_ON_CUDA(cudaResult, NAMEOF(cuMemAlloc) + L" failed!");

	CUDA_MEMCPY2D memCopy2D;
	ZeroMemory(&memCopy2D, sizeof(memCopy2D));
	memCopy2D.srcMemoryType = CU_MEMORYTYPE_ARRAY;
	memCopy2D.dstMemoryType = CU_MEMORYTYPE_DEVICE;
	memCopy2D.dstDevice = cudaData;
	memCopy2D.srcArray = cudaArray;
	memCopy2D.dstPitch = elementSizeInBytes * c * w;
	memCopy2D.srcXInBytes = 0;
	memCopy2D.srcY = 0;
	memCopy2D.dstXInBytes = 0;
	memCopy2D.dstY = 0;
	memCopy2D.WidthInBytes = elementSizeInBytes * c * w;
	memCopy2D.Height = h;

	cudaResult = cuMemcpy2D(&memCopy2D);
	ERROR_MESSAGE_ON_CUDA(cudaResult, NAMEOF(cuMemcpy2D) + L" failed!");

	// Create a Pytorch tensor from the CUDA memory pointer
	at::Tensor tensor = torch::from_blob((void*)cudaData, { 1, h, w, c }, c10::TensorOptions().device(torch::kCUDA).dtype(scalarType).layout(torch::kStrided));

	// Clone the tensor such that it no longer uses the manually allocated CUDA memory (and there is no need to explicitly free it after this function exits)
	tensor = tensor.clone();

	cudaResult = cuGraphicsUnmapResources(1, &cudaGraphicsResource, cudaStream);
	ERROR_MESSAGE_ON_CUDA(cudaResult, NAMEOF(cuGraphicsUnmapResources) + L" failed!");

	cudaResult = cuMemFree(cudaData);
	ERROR_MESSAGE_ON_CUDA(cudaResult, NAMEOF(cuMemFree) + L" failed!");

	return tensor;
}
