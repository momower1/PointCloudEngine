#include "Octree.hlsl"
#include "OctreeConstantBuffer.hlsl"

VS_INPUT VS(VS_INPUT input)
{
    return input;
}

[maxvertexcount(8)]
void GS(point VS_INPUT input[1], inout LineStream<GS_OUTPUT> output)
{
    float extend = 0.5f * input[0].size;

    float4x4 WVP = mul(World, mul(View, Projection));

	float weights[4] =
	{
		input[0].weight0 / 255.0f,
		input[0].weight1 / 255.0f,
		input[0].weight2 / 255.0f,
		1.0f - (input[0].weight0 + input[0].weight1 + input[0].weight2) / 255.0f
	};

	// Scale the length of the normal (sum of all weights is one)
    float maxWeight = max(weights[0], max(weights[1], max(weights[2], weights[3])));

	// Store all of the end vertices for the normals here for readability
    float3 end[] =
    {
        input[0].position + extend * (weights[0] / maxWeight) * PolarNormalToFloat3(input[0].normal0),
        input[0].position + extend * (weights[1] / maxWeight) * PolarNormalToFloat3(input[0].normal1),
        input[0].position + extend * (weights[2] / maxWeight) * PolarNormalToFloat3(input[0].normal2),
        input[0].position + extend * (weights[3] / maxWeight) * PolarNormalToFloat3(input[0].normal3),
    };

    GS_OUTPUT element;
    element.normal = float3(0, 0, 0);

	// Append a line from the center to the end of each normal
    element.position = element.clipPosition = mul(float4(input[0].position, 1), WVP);
    element.color = Color16ToFloat3(input[0].color0);
    output.Append(element);
    element.position = mul(float4(end[0], 1), WVP);
    output.Append(element);
    output.RestartStrip();

    element.position = element.clipPosition = mul(float4(input[0].position, 1), WVP);
    element.color = Color16ToFloat3(input[0].color1);
    output.Append(element);
    element.position = mul(float4(end[1], 1), WVP);
    output.Append(element);
    output.RestartStrip();

    element.position = element.clipPosition = mul(float4(input[0].position, 1), WVP);
    element.color = Color16ToFloat3(input[0].color2);
    output.Append(element);
    element.position = mul(float4(end[2], 1), WVP);
    output.Append(element);
    output.RestartStrip();

    element.position = element.clipPosition = mul(float4(input[0].position, 1), WVP);
    element.color = Color16ToFloat3(input[0].color3);
    output.Append(element);
    element.position = mul(float4(end[3], 1), WVP);
    output.Append(element);
    output.RestartStrip();
}

float4 PS(GS_OUTPUT input) : SV_TARGET
{
    return float4(input.color.rgb, 1);
}