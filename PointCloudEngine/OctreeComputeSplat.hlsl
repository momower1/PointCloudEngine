#include "OctreeSplatGS.hlsl"

StructuredBuffer<OctreeNodeVertex> vertexBuffer : register(t0);

VS_OUTPUT VS(uint vertexID : SV_VERTEXID)
{
    OctreeNodeVertex v = vertexBuffer[vertexID];

    VS_INPUT input;

    input.position = v.position;
    input.normal0 = v.normals[0] >> 16;
    input.normal1 = v.normals[0] & 0xffff;
    input.normal2 = v.normals[1] >> 16;
    input.normal3 = v.normals[1] & 0xffff;
    input.normal4 = v.normals[2] >> 16;
    input.normal5 = v.normals[2] & 0xffff;
    input.color0 = v.colors[0] >> 16;
    input.color1 = v.colors[0] & 0xffff;
    input.color2 = v.colors[1] >> 16;
    input.color3 = v.colors[1] & 0xffff;
    input.color4 = v.colors[2] >> 16;
    input.color5 = v.colors[2] & 0xffff;
    input.weights = v.weights;
    input.size = v.size;

    return VertexShaderFunction(input);
}

float4 PS(GS_OUTPUT input) : SV_TARGET
{
    return PixelShaderFunction(input);
}