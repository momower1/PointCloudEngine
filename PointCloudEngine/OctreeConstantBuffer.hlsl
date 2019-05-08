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
	float4x4 WorldViewProjectionInverse;
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
	float3 localViewFrustumNearTopRight;
	// 4 byte auto padding
//------------------------------------------------------------------------------ (16 byte boundary)
	float3 localViewFrustumNearBottomLeft;
	// 4 byte auto padding
//------------------------------------------------------------------------------ (16 byte boundary)
	float3 localViewFrustumNearBottomRight;
	// 4 byte auto padding
//------------------------------------------------------------------------------ (16 byte boundary)
	float3 localViewFrustumFarTopLeft;
	// 4 byte auto padding
//------------------------------------------------------------------------------ (16 byte boundary)
	float3 localViewFrustumFarTopRight;
	// 4 byte auto padding
//------------------------------------------------------------------------------ (16 byte boundary)
	float3 localViewFrustumFarBottomLeft;
	// 4 byte auto padding
//------------------------------------------------------------------------------ (16 byte boundary)
	float3 localViewFrustumFarBottomRight;
	// 4 byte auto padding
//------------------------------------------------------------------------------ (16 byte boundary)
	float3 localViewPlaneNearNormal;
	// 4 byte auto padding
//------------------------------------------------------------------------------ (16 byte boundary)
	float3 localViewPlaneFarNormal;
	// 4 byte auto padding
//------------------------------------------------------------------------------ (16 byte boundary)
	float3 localViewPlaneLeftNormal;
	// 4 byte auto padding
//------------------------------------------------------------------------------ (16 byte boundary)
	float3 localViewPlaneRightNormal;
	// 4 byte auto padding
//------------------------------------------------------------------------------ (16 byte boundary)
	float3 localViewPlaneTopNormal;
	// 4 byte auto padding
//------------------------------------------------------------------------------ (16 byte boundary)
	float3 localViewPlaneBottomNormal;
	float fovAngleY;
//------------------------------------------------------------------------------ (16 byte boundary)
	float splatSize;
	float samplingRate;
	float overlapFactor;
	int level;
//------------------------------------------------------------------------------ (16 byte boundary)
	// Lighting
	bool light;
	float3 lightDirection;
//------------------------------------------------------------------------------ (16 byte boundary)
	float lightIntensity;
	float ambient;
	float diffuse;
	float specular;
//------------------------------------------------------------------------------ (16 byte boundary)
	float specularExponent;
	// Blending
	bool blend;
	float depthEpsilon;
	// Compute shader data
	uint inputCount;
//------------------------------------------------------------------------------ (16 byte boundary)
};	// Total: 640 bytes with constant buffer packing rules