cbuffer GroundTruthConstantBuffer : register(b0)
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
	float4x4 PreviousWorld;
	//------------------------------------------------------------------------------ (64 byte boundary)
	float4x4 PreviousView;
	//------------------------------------------------------------------------------ (64 byte boundary)
	float4x4 PreviousProjection;
	//------------------------------------------------------------------------------ (64 byte boundary)
	float4x4 PreviousWorldInverseTranspose;
	//------------------------------------------------------------------------------ (64 byte boundary)
	float3 cameraPosition;
	float fovAngleY;
	//------------------------------------------------------------------------------ (16 byte boundary)
	float samplingRate;
	float blendFactor;
	bool useBlending;
	bool backfaceCulling;
	//------------------------------------------------------------------------------ (16 byte boundary)
	int shadingMode;
	int textureLOD;
	int resolutionX;
	int resolutionY;
	//------------------------------------------------------------------------------ (16 byte boundary)
};  // Total: 372 bytes with constant buffer packing rules