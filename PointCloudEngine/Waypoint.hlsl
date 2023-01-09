#include "GroundTruthConstantBuffer.hlsli"

struct VS_INPUT
{
	float3 position : POSITION;
	float3 color : COLOR;
};

struct VS_OUTPUT
{
	float4 position : SV_POSITION;
	float3 color : COLOR;
};

VS_OUTPUT VS(VS_INPUT input)
{
	VS_OUTPUT output;

	float4x4 VP = mul(View, Projection);
	output.position = mul(float4(input.position, 1), VP);
	output.color = input.color;

	return output;
}

float4 PS(VS_OUTPUT input) : SV_TARGET
{
	return float4(input.color, 1);
}