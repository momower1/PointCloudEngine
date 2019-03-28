cbuffer OctreeRendererConstantBuffer : register(b0)
{
    float4x4 World;
//------------------------------------------------------------------------------ (64 byte boundary)
    float4x4 View;
//------------------------------------------------------------------------------ (64 byte boundary)
    float4x4 Projection;
//------------------------------------------------------------------------------ (64 byte boundary)
    float4x4 WorldInverseTranspose;
//------------------------------------------------------------------------------ (64 byte boundary)
    float3 cameraPosition;
    int viewDirectionIndex;
//------------------------------------------------------------------------------ (16 byte boundary)
    float fovAngleY;
    float splatSize;
    // 8 bytes auto padding
};  // Total: 288 bytes with constant buffer packing rules

static const float PI = 3.141592654f;

static const float3 viewDirections[6] =
{
    float3(1.0f, 0.0f, 0.0f),
    float3(-1.0f, 0.0f, 0.0f),
    float3(0.0f, 1.0f, 0.0f),
    float3(0.0f, -1.0f, 0.0f),
    float3(0.0f, 0.0f, 1.0f),
    float3(0.0f, 0.0f, -1.0f)
};

struct VS_INPUT
{
    float3 position : POSITION;
    uint2 normal0 : NORMAL0;
    uint2 normal1 : NORMAL1;
    uint2 normal2 : NORMAL2;
    uint2 normal3 : NORMAL3;
    uint2 normal4 : NORMAL4;
    uint2 normal5 : NORMAL5;
    uint4 color0 : COLOR0;
    uint4 color1 : COLOR1;
    uint4 color2 : COLOR2;
    uint4 color3 : COLOR3;
    uint4 color4 : COLOR4;
    uint4 color5 : COLOR5;
    float size : SIZE;
};

struct VS_OUTPUT
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float size : SIZE;
    float4 color : COLOR;
};

struct GS_OUTPUT
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float4 color : COLOR;
};

float3 PolarNormalToFloat3(uint theta, uint phi)
{
    float t = PI * (theta / 255.0f);
    float p = PI * ((phi / 128.0f) - 1.0f);

    return float3(sin(t) * cos(p), sin(t) * sin(p), cos(t));
}

float3 PolarNormalToFloat3(uint2 polarNormal)
{
    return PolarNormalToFloat3(polarNormal.x, polarNormal.y);
}

VS_OUTPUT VS(VS_INPUT input)
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
    
    float4 colors[6] =
    {
        input.color0 / 255.0f,
        input.color1 / 255.0f,
        input.color2 / 255.0f,
        input.color3 / 255.0f,
        input.color4 / 255.0f,
        input.color5 / 255.0f
    };

    VS_OUTPUT output;
    output.position = input.position;
    output.size = input.size;

    // Use world inverse to transform camera position into local space
    float4x4 worldInverse = transpose(WorldInverseTranspose);

    // View direction in local space
    float3 viewDirection = input.position - mul(float4(cameraPosition, 1), worldInverse);
    float3 normal = float3(0, 0, 0);
    float4 color = float4(0, 0, 0, 0);

    if (viewDirectionIndex >= 0)
    {
        viewDirection = viewDirections[viewDirectionIndex];
    }

    viewDirection = normalize(viewDirection);

    float visibilityFactorSum = 0;

    // Compute the normal and color from this view direction
    for (int i = 0; i < 6; i++)
    {
        float visibilityFactor = dot(normals[i], -viewDirection);

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

float4 PS(GS_OUTPUT input) : SV_TARGET
{
    return input.color;
}