#define PI 3.141592654f

cbuffer GroundTruthRendererConstantBuffer : register(b0)
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
    float fovAngleY;
//------------------------------------------------------------------------------ (16 byte boundary)
    float samplingRate;
	float blendFactor;
	bool useBlending;
	bool drawNormals;
//------------------------------------------------------------------------------ (16 byte boundary)
	bool normalsInScreenSpace;
	bool backfaceCulling;
	// 8 bytes auto padding
//------------------------------------------------------------------------------ (16 byte boundary)
};  // Total: 368 bytes with constant buffer packing rules

struct VS_INPUT
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    uint3 color : COLOR;
};

struct VS_OUTPUT
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float3 color : COLOR;
};

VS_OUTPUT VS(VS_INPUT input)
{
	VS_OUTPUT output;
	output.position = mul(float4(input.position, 1), World);
	output.normal = normalize(mul(input.normal, WorldInverseTranspose));
	output.color = input.color / 255.0f;

	return output;
}