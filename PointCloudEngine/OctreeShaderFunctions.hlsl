#include "Octree.hlsl"

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

VS_INPUT VertexShaderFunction(VS_INPUT input)
{
    return input;
}

float4 PixelShaderFunction(GS_OUTPUT input)
{
    return float4(input.color, 1);
}