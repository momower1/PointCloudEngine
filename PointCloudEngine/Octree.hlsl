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
};  // Total: 272 bytes with constant buffer packing rules

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
    float3 normal0 : NORMAL0;
    float3 normal1 : NORMAL1;
    float3 normal2 : NORMAL2;
    float3 normal3 : NORMAL3;
    float3 normal4 : NORMAL4;
    float3 normal5 : NORMAL5;
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
    float4 color : COLOR;
};

VS_OUTPUT VS(VS_INPUT input)
{
    float3 normals[6] =
    {
        input.normal0,
        input.normal1,
        input.normal2,
        input.normal3,
        input.normal4,
        input.normal5
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
        float visibilityFactor = max(0, dot(normals[i], -viewDirection));

        normal += visibilityFactor * normals[i];
        color += visibilityFactor * colors[i];

        visibilityFactorSum += visibilityFactor;
    }

    normal /= visibilityFactorSum;
    color /= visibilityFactorSum;

    output.normal = normalize(mul(normal, WorldInverseTranspose));
    output.color = color;

    return output;
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
    return input.color;
}