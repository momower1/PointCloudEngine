#include "OctreeShaderFunctions.hlsl"

VS_OUTPUT VS(VS_INPUT input)
{
    return VertexShaderFunction(input);
}

[maxvertexcount(16)]
void GS(point VS_OUTPUT input[1], inout LineStream<GS_OUTPUT> output)
{
    float extend = 0.5f * input[0].size;
    float3 x = float3(extend, 0, 0);
    float3 y = float3(0, extend, 0);
    float3 z = float3(0, 0, extend);

    float4x4 WVP = mul(World, mul(View, Projection));

    GS_OUTPUT element;
    element.color = input[0].color;
    element.normal = input[0].normal;

    float3 cube[] =
    {
        input[0].position - x - y - z,
        input[0].position + x - y - z,
        input[0].position + x + y - z,
        input[0].position - x + y - z,
        input[0].position - x + y + z,
        input[0].position - x - y + z,
        input[0].position + x - y + z,
        input[0].position + x + y + z
    };

    element.position = mul(float4(cube[3], 1), WVP);
    output.Append(element);
    element.position = mul(float4(cube[0], 1), WVP);
    output.Append(element);
    element.position = mul(float4(cube[1], 1), WVP);
    output.Append(element);
    element.position = mul(float4(cube[2], 1), WVP);
    output.Append(element);
    element.position = mul(float4(cube[3], 1), WVP);
    output.Append(element);
    element.position = mul(float4(cube[4], 1), WVP);
    output.Append(element);
    element.position = mul(float4(cube[5], 1), WVP);
    output.Append(element);
    element.position = mul(float4(cube[6], 1), WVP);
    output.Append(element);
    element.position = mul(float4(cube[7], 1), WVP);
    output.Append(element);
    element.position = mul(float4(cube[4], 1), WVP);
    output.Append(element);

    output.RestartStrip();

    element.position = mul(float4(cube[2], 1), WVP);
    output.Append(element);
    element.position = mul(float4(cube[7], 1), WVP);
    output.Append(element);

    output.RestartStrip();

    element.position = mul(float4(cube[1], 1), WVP);
    output.Append(element);
    element.position = mul(float4(cube[6], 1), WVP);
    output.Append(element);

    output.RestartStrip();

    element.position = mul(float4(cube[0], 1), WVP);
    output.Append(element);
    element.position = mul(float4(cube[5], 1), WVP);
    output.Append(element);
}

float4 PS(GS_OUTPUT input) : SV_TARGET
{
    return PixelShaderFunction(input);
}