#include "GroundTruthConstantBuffer.hlsli"

#define PI 3.141592654f

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
	output.position = mul(float4(input.position, 1), World).xyz;
	output.normal = normalize(mul(float4(input.normal, 0), WorldInverseTranspose)).xyz;
	output.color = input.color / 255.0f;

	return output;
}