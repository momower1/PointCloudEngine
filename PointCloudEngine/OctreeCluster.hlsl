#include "Octree.hlsl"

struct GS_OUTPUT
{
    float4 position : SV_POSITION;
    float3 color : COLOR;
};

VS_INPUT VS(VS_INPUT input)
{
    return input;
}

[maxvertexcount(12)]
void GS(point VS_INPUT input[1], inout LineStream<GS_OUTPUT> output)
{
    float extend = 0.5f * input[0].size;

    float4x4 WVP = mul(World, mul(View, Projection));

    float maxWeight = max(input[0].weight0, max(input[0].weight1, max(input[0].weight2, max(input[0].weight3, max(input[0].weight4, input[0].weight5)))));

    float3 end[] =
    {
        input[0].position + extend * (input[0].weight0 / maxWeight) * PolarNormalToFloat3(input[0].normal0),
        input[0].position + extend * (input[0].weight1 / maxWeight) * PolarNormalToFloat3(input[0].normal1),
        input[0].position + extend * (input[0].weight2 / maxWeight) * PolarNormalToFloat3(input[0].normal2),
        input[0].position + extend * (input[0].weight3 / maxWeight) * PolarNormalToFloat3(input[0].normal3),
        input[0].position + extend * (input[0].weight4 / maxWeight) * PolarNormalToFloat3(input[0].normal4),
        input[0].position + extend * (input[0].weight5 / maxWeight) * PolarNormalToFloat3(input[0].normal5)
    };

    GS_OUTPUT element;
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
    return float4(input.color, 1);
}