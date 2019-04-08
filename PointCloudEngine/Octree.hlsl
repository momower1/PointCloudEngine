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
    float fovAngleY;
//------------------------------------------------------------------------------ (16 byte boundary)
    float splatSize;
    // 12 bytes auto padding
};  // Total: 288 bytes with constant buffer packing rules

static const float PI = 3.141592654f;

struct VS_INPUT
{
    float3 position : POSITION;
    uint2 normal0 : NORMAL0;
    uint2 normal1 : NORMAL1;
    uint2 normal2 : NORMAL2;
    uint2 normal3 : NORMAL3;
    uint2 normal4 : NORMAL4;
    uint2 normal5 : NORMAL5;
    uint color0 : COLOR0;
    uint color1 : COLOR1;
    uint color2 : COLOR2;
    uint color3 : COLOR3;
    uint color4 : COLOR4;
    uint color5 : COLOR5;
    uint weight0 : WEIGHT0;
    uint weight1 : WEIGHT1;
    uint weight2 : WEIGHT2;
    uint weight3 : WEIGHT3;
    uint weight4 : WEIGHT4;
    uint weight5 : WEIGHT5;
    float size : SIZE;
};

float3 PolarNormalToFloat3(uint theta, uint phi)
{
    if (theta == 0 && phi == 0)
    {
        return float3(0, 0, 0);
    }

    float t = PI * (theta / 255.0f);
    float p = PI * ((phi / 127.5f) - 1.0f);

    return float3(sin(t) * cos(p), sin(t) * sin(p), cos(t));
}

float3 PolarNormalToFloat3(uint2 polarNormal)
{
    return PolarNormalToFloat3(polarNormal.x, polarNormal.y);
}

float3 Color16ToFloat3(uint color)
{
    float r = ((color >> 10) & 63) / 63.0f;
    float g = ((color >> 4) & 63) / 63.0f;
    float b = (color & 15) / 15.0f;

    return float3(r, g, b);
}