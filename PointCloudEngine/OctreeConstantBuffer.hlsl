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
	float3 localViewFrustumNearTopLeft;
	// 4 byte auto padding
//------------------------------------------------------------------------------ (16 byte boundary)
	float3 localViewFrustumFarBottomRight;
	// 4 byte auto padding
//------------------------------------------------------------------------------ (16 byte boundary)
	float3 localViewFrustumNearNormal;
	// 4 byte auto padding
//------------------------------------------------------------------------------ (16 byte boundary)
	float3 localViewFrustumFarNormal;
	// 4 byte auto padding
//------------------------------------------------------------------------------ (16 byte boundary)
	float3 localViewFrustumLeftNormal;
	// 4 byte auto padding
//------------------------------------------------------------------------------ (16 byte boundary)
	float3 localViewFrustumRightNormal;
	// 4 byte auto padding
//------------------------------------------------------------------------------ (16 byte boundary)
	float3 localViewFrustumTopNormal;
	// 4 byte auto padding
//------------------------------------------------------------------------------ (16 byte boundary)
	float3 localViewFrustumBottomNormal;
	float fovAngleY;
//------------------------------------------------------------------------------ (16 byte boundary)
	float splatSize;
	float samplingRate;
	float overlapFactor;
	int level;
//------------------------------------------------------------------------------ (16 byte boundary)

	// Compute shader data
	uint inputCount;

	// 12 byte auto padding
//------------------------------------------------------------------------------ (16 byte boundary)
};	// Total: 448 bytes with constant buffer packing rules