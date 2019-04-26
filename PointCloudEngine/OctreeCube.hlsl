#include "OctreeShaderFunctions.hlsl"

VS_INPUT VS(VS_INPUT input)
{
    return VertexShaderFunction(input);
}

[maxvertexcount(16)]
void GS(point VS_INPUT input[1], inout LineStream<GS_OUTPUT> output)
{
	float3 colors[4] =
	{
		Color16ToFloat3(input[0].color0),
		Color16ToFloat3(input[0].color1),
		Color16ToFloat3(input[0].color2),
		Color16ToFloat3(input[0].color3)
	};

	float weights[4] =
	{
		input[0].weight0 / 255.0f,
		input[0].weight1 / 255.0f,
		input[0].weight2 / 255.0f,
		1.0f - (input[0].weight0 + input[0].weight1 + input[0].weight2) / 255.0f
	};

    float extend = 0.5f * input[0].size;
    float3 x = float3(extend, 0, 0);
    float3 y = float3(0, extend, 0);
    float3 z = float3(0, 0, extend);

    float4x4 WVP = mul(World, mul(View, Projection));

    GS_OUTPUT element;
	element.color = float3(0, 0, 0);
	element.normal = float3(0, 0, 0);

	// Assign the average color from all the points inside this cube
	for (int i = 0; i < 4; i++)
	{
		element.color += weights[i] * colors[i];
	}

	// Store all the cube vertices here for readability
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

	// Append in such a way that the lowest possible number of vertices is used
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