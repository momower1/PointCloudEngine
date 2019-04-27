cbuffer OctreeConstantBuffer : register(b0)
{
	float4x4 World;
//------------------------------------------------------------------------------ (64 byte boundary)
	float4x4 View;
//------------------------------------------------------------------------------ (64 byte boundary)
	float4x4 Projection;
//------------------------------------------------------------------------------ (64 byte boundary)
	float4x4 WorldInverseTranspose;
//------------------------------------------------------------------------------ (64 byte boundary)
	float3 cameraPosition;
	// 4 byte auto padding
//------------------------------------------------------------------------------ (16 byte boundary)
	float3 localCameraPosition;
	// 4 byte auto padding
//------------------------------------------------------------------------------ (16 byte boundary)
	float4 localViewFrustum[8];
//------------------------------------------------------------------------------ (128 byte boundary)
	float fovAngleY;
	float splatSize;
	float samplingRate;
	float overlapFactor;
//------------------------------------------------------------------------------ (16 byte boundary)
	int level;

	// Compute shader data
	uint inputCount;

	// 8 byte auto padding
//------------------------------------------------------------------------------ (16 byte boundary)
};	// Total: 448 bytes with constant buffer packing rules