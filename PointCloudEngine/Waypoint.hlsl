cbuffer WaypointRendererConstantBuffer : register(b0)
{
	float4x4 View;
	//------------------------------------------------------------------------------ (64 byte boundary)
	float4x4 Projection;
	//------------------------------------------------------------------------------ (64 byte boundary)
};  // Total: 128 bytes with constant buffer packing rules

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