#include "Octree.hlsl"

struct VS_OUTPUT
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float size : SIZE;
    float3 color : COLOR;
};

struct GS_OUTPUT
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float3 color : COLOR;
};

VS_OUTPUT VertexShaderFunction(VS_INPUT input)
{
    float3 normals[6] =
    {
        PolarNormalToFloat3(input.normal0),
        PolarNormalToFloat3(input.normal1),
        PolarNormalToFloat3(input.normal2),
        PolarNormalToFloat3(input.normal3),
        PolarNormalToFloat3(input.normal4),
        PolarNormalToFloat3(input.normal5)
    };

    float3 colors[6] =
    {
        Color16ToFloat3(input.color0),
        Color16ToFloat3(input.color1),
        Color16ToFloat3(input.color2),
        Color16ToFloat3(input.color3),
        Color16ToFloat3(input.color4),
        Color16ToFloat3(input.color5)
    };

    float weights[6] =
    {
        (input.weights & 31) / 31.0f,
        ((input.weights >> 5) & 31) / 31.0f,
        ((input.weights >> 10) & 31) / 31.0f,
        ((input.weights >> 15) & 31) / 31.0f,
        ((input.weights >> 20) & 31) / 31.0f,
        ((input.weights >> 25) & 31) / 31.0f
    };

    VS_OUTPUT output;
    output.position = input.position;
    output.size = input.size;

    // Use world inverse to transform camera position into local space
    float4x4 worldInverse = transpose(WorldInverseTranspose);

    // View direction in local space
    float3 viewDirection = normalize(input.position - mul(float4(cameraPosition, 1), worldInverse));
    float3 normal = float3(0, 0, 0);
    float3 color = float3(0, 0, 0);

    float visibilityFactorSum = 0;

    // Compute the normal and color from this view direction
    for (int i = 0; i < 6; i++)
    {
        float visibilityFactor = weights[i] * dot(normals[i], -viewDirection);

        if (visibilityFactor > 0)
        {
            normal += visibilityFactor * normals[i];
            color += visibilityFactor * colors[i];

            visibilityFactorSum += visibilityFactor;
        }
    }

    normal /= visibilityFactorSum;
    color /= visibilityFactorSum;

    output.normal = normalize(normal);
    output.color = color;

    return output;
}

float4 PixelShaderFunction(GS_OUTPUT input)
{
    return float4(input.color, 1);
}