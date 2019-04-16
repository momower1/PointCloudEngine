#include "OctreeShaderFunctions.hlsl"

StructuredBuffer<OctreeNode> nodesBuffer : register(t0);
StructuredBuffer<uint> vertexBuffer : register(t1);

VS_OUTPUT VS(uint vertexID : SV_VERTEXID)
{
    uint index = vertexBuffer[vertexID];

    OctreeNodeVertex v = nodesBuffer[index].nodeVertex;

    VS_INPUT input;

    input.position = v.position;
    input.normal0 = uint2(v.normal01 >> 24, (v.normal01 >> 16) & 0xff);
    input.normal1 = uint2((v.normal01 >> 8) & 0xff, v.normal01 & 0xff);
    input.normal2 = uint2(v.normal23 >> 24, (v.normal23 >> 16) & 0xff);
    input.normal3 = uint2((v.normal23 >> 8) & 0xff, v.normal23 & 0xff);
    input.normal4 = uint2(v.normal45 >> 24, (v.normal45 >> 16) & 0xff);
    input.normal5 = uint2((v.normal45 >> 8) & 0xff, v.normal45 & 0xff);
    input.color0 = v.color01 >> 16;
    input.color1 = v.color01 & 0xffff;
    input.color2 = v.color23 >> 16;
    input.color3 = v.color23 & 0xffff;
    input.color4 = v.color45 >> 16;
    input.color5 = v.color45 & 0xffff;
    input.weights = v.weights;
    input.size = v.size;

    return VertexShaderFunction(input);
}