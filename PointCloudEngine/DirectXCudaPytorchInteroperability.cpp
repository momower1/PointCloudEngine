#include "PrecompiledHeader.h"
#include "DirectXCudaPytorchInteroperability.h"

bool DirectXCudaPytorchInteroperability::initialized = false;
CUresult DirectXCudaPytorchInteroperability::cudaResult;
CUdevice DirectXCudaPytorchInteroperability::cudaDevice;
CUcontext DirectXCudaPytorchInteroperability::cudaContext = NULL;
CUstream DirectXCudaPytorchInteroperability::cudaStream = NULL;
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

	if (cudaGraphicsResourceBackbuffer == NULL)
	{
		// Register a shared handle to the DirectX backbuffer texture
		cudaResult = cuGraphicsD3D11RegisterResource(&cudaGraphicsResourceBackbuffer, backBufferTexture, CU_GRAPHICS_REGISTER_FLAGS_NONE);
		ERROR_MESSAGE_ON_CUDA(cudaResult, NAMEOF(cuGraphicsD3D11RegisterResource) + L" failed!");
	}
}

void DirectXCudaPytorchInteroperability::ReleaseSharedResources()
{
	if (cudaGraphicsResourceBackbuffer != NULL)
	{
		cudaResult = cuGraphicsUnregisterResource(cudaGraphicsResourceBackbuffer);
		ERROR_MESSAGE_ON_CUDA(cudaResult, NAMEOF(cuGraphicsUnregisterResource) + L" failed!");
		cudaGraphicsResourceBackbuffer = NULL;
	}
}

void DirectXCudaPytorchInteroperability::Execute()
{
	// TODO: Possibly the mapping is not even necessary, depending on how a tensor is created in Pytorch
	cudaResult = cuGraphicsMapResources(1, &cudaGraphicsResourceBackbuffer, cudaStream);
	ERROR_MESSAGE_ON_CUDA(cudaResult, NAMEOF(cuGraphicsMapResources) + L" failed!");

	// Map the texture resource to an array
	CUarray cudaArray;
	cudaResult = cuGraphicsSubResourceGetMappedArray(&cudaArray, cudaGraphicsResourceBackbuffer, 0, 0);
	ERROR_MESSAGE_ON_CUDA(cudaResult, NAMEOF(cuGraphicsSubResourceGetMappedArray) + L" failed!");

	// TODO: Try directly taking the pointer instead to avoid additional memory copy (but apparently there will be errors due to texture layout)
	// TODO: For the model, actually only share a single buffer instead of a texture
	//cuGraphicsResourceGetMappedPointer()

	CUDA_ARRAY_DESCRIPTOR cudaArrayDescriptor;
	cudaResult = cuArrayGetDescriptor(&cudaArrayDescriptor, cudaArray);

	long long elementSizeInBytes = 2;
	long long c = cudaArrayDescriptor.NumChannels;
	long long h = cudaArrayDescriptor.Height;
	long long w = cudaArrayDescriptor.Width;

	// Need to allocate and copy to a cuda device pointer that can be used in pytorch
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
	at::Tensor tensor = torch::from_blob((void*)cudaData, { 1, h, w, c }, c10::TensorOptions().device(torch::kCUDA).dtype(torch::kF16).layout(torch::kStrided));

	// Perform operations on the tensor
	tensor.index_put_({ at::indexing::Slice(), at::indexing::Slice(), at::indexing::Slice(0, w / 2), at::indexing::Slice() }, 0.0f);
	tensor.index_put_({ at::indexing::Slice(), at::indexing::Slice(0, h / 3), at::indexing::Slice(0, w / 2), at::indexing::Slice(0, 1) }, 1.0f);
	tensor.index_put_({ at::indexing::Slice(), at::indexing::Slice(h / 3, (2 * h) / 3), at::indexing::Slice(0, w / 2), at::indexing::Slice(1, 2) }, 1.0f);
	tensor.index_put_({ at::indexing::Slice(), at::indexing::Slice((2 * h) / 3, h), at::indexing::Slice(0, w / 2), at::indexing::Slice(2, 3) }, 1.0f);
	//tensor.index_put_({ at::indexing::Slice(), at::indexing::Slice(), at::indexing::Slice(w / 2, w), at::indexing::Slice() }, 1.0f);

	// Copy back into CUDA array
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

	cudaResult = cuGraphicsUnmapResources(1, &cudaGraphicsResourceBackbuffer, cudaStream);
	ERROR_MESSAGE_ON_CUDA(cudaResult, NAMEOF(cuGraphicsUnmapResources) + L" failed!");

	cudaResult = cuMemFree(cudaData);
	ERROR_MESSAGE_ON_CUDA(cudaResult, NAMEOF(cuMemFree) + L" failed!");
}
