#include "OctreeShaderFunctions.hlsl"

VS_INPUT VS(VS_INPUT input)
{
    return input;
}

[maxvertexcount(12)]
void GS(point VS_INPUT input[1], inout LineStream<GS_OUTPUT> output)
{
    float extend = 0.5f * input[0].size;

    float4x4 WVP = mul(World, mul(View, Projection));

    float weights[6] =
    {
        (input[0].weights & 31) / 31.0f,
        ((input[0].weights >> 5) & 31) / 31.0f,
        ((input[0].weights >> 10) & 31) / 31.0f,
        ((input[0].weights >> 15) & 31) / 31.0f,
        ((input[0].weights >> 20) & 31) / 31.0f,
        ((input[0].weights >> 25) & 31) / 31.0f
    };

	// Scale the length of the normal (sum of all weights is one)
    float maxWeight = max(weights[0], max(weights[1], max(weights[2], max(weights[3], max(weights[4], weights[5])))));

	// Store all of the end vertices for the normals here for readability
    float3 end[] =
    {
        input[0].position + extend * (weights[0] / maxWeight) * PolarNormalToFloat3(input[0].normal0),
        input[0].position + extend * (weights[1] / maxWeight) * PolarNormalToFloat3(input[0].normal1),
        input[0].position + extend * (weights[2] / maxWeight) * PolarNormalToFloat3(input[0].normal2),
        input[0].position + extend * (weights[3] / maxWeight) * PolarNormalToFloat3(input[0].normal3),
        input[0].position + extend * (weights[4] / maxWeight) * PolarNormalToFloat3(input[0].normal4),
        input[0].position + extend * (weights[5] / maxWeight) * PolarNormalToFloat3(input[0].normal5)
    };

    GS_OUTPUT element;
    element.normal = float3(0, 0, 0);

	// Append a line from the center to the end of each normal
    element.position = mul(float4(input[0].position, 1), WVP);
    element.color = Color16ToFloat3(input[0].color0);
    output.Append(element);
    element.position = mul(float4(end[0], 1), WVP);
    output.Append(element);
    output.RestartStrip();

    element.position = mul(float4(input[0].position, 1), WVP);
    element.color = Color16ToFloat3(input[0].color1);
    output.Append(element);
    element.position = mul(float4(end[1], 1), WVP);
    output.Append(element);
    output.RestartStrip();

    element.position = mul(float4(input[0].position, 1), WVP);
    element.color = Color16ToFloat3(input[0].color2);
    output.Append(element);
    element.position = mul(float4(end[2], 1), WVP);
    output.Append(element);
    output.RestartStrip();

    element.position = mul(float4(input[0].position, 1), WVP);
    element.color = Color16ToFloat3(input[0].color3);
    output.Append(element);
    element.position = mul(float4(end[3], 1), WVP);
    output.Append(element);
    output.RestartStrip();

    element.position = mul(float4(input[0].position, 1), WVP);
    element.color = Color16ToFloat3(input[0].color4);
    output.Append(element);
    element.position = mul(float4(end[4], 1), WVP);
    output.Append(element);
    output.RestartStrip();

    element.position = mul(float4(input[0].position, 1), WVP);
    element.color = Color16ToFloat3(input[0].color5);
    output.Append(element);
    element.position = mul(float4(end[5], 1), WVP);
    output.Append(element);
    output.RestartStrip();
}

float4 PS(GS_OUTPUT input) : SV_TARGET
{
    return PixelShaderFunction(input);
}