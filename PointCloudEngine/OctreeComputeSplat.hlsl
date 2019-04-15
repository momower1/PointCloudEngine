#include "OctreeSplatGS.hlsl"

StructuredBuffer<OctreeNode> nodesBuffer : register(t0);
StructuredBuffer<uint> vertexBuffer : register(t1);

VS_OUTPUT VS(uint vertexID : SV_VERTEXID)
{
    uint index = vertexBuffer[vertexID];

    OctreeNodeVertex v = nodesBuffer[index].nodeVertex;

    VS_INPUT input;

    input.position = v.position;
    input.normal0 = uint2(v.normals[0] >> 24, (v.normals[0] >> 16) & 0xff);
    input.normal1 = uint2((v.normals[0] >> 8) & 0xff, v.normals[0] & 0xff);
    input.normal2 = uint2(v.normals[1] >> 24, (v.normals[1] >> 16) & 0xff);
    input.normal3 = uint2((v.normals[1] >> 8) & 0xff, v.normals[1] & 0xff);
    input.normal4 = uint2(v.normals[2] >> 24, (v.normals[2] >> 16) & 0xff);
    input.normal5 = uint2((v.normals[2] >> 8) & 0xff, v.normals[2] & 0xff);
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