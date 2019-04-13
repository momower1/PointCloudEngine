#include "Common.hlsl"

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
    float overlapFactor;
    // 8 bytes auto padding
};  // Total: 288 bytes with constant buffer packing rules

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
    uint weights : WEIGHTS;
    float size : SIZE;
};