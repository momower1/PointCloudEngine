#include "Common.hlsl"

cbuffer ComputeShaderConstantBuffer : register(b0)
{
    float3 localCameraPosition;
    float splatSize;
//------------------------------------------------------------------------------ (16 byte boundary)
};  // Total: 16 bytes with constant buffer packing rules

struct OctreeNodeVertex
{
    float3 position;
    half normals[6];
    half colors[6];
    uint weights;
    float size;
};

struct OctreeNode
{
    int children[8];
    OctreeNodeVertex nodeVertex;
};

StructuredBuffer<OctreeNode> nodesBuffer : register(t0);
ConsumeStructuredBuffer<int> inputBuffer : register(u0);
AppendStructuredBuffer<int> outputBuffer : register(u1);
AppendStructuredBuffer<OctreeNodeVertex> vertexBuffer : register(u2);

[numthreads(1, 1, 1)]
void CS (uint3 id : SV_DispatchThreadID )
{
    //int index = inputBuffer.Consume();

    //// TODO: Remove
    //OctreeNode n = nodesBuffer[index];

    //float3 color = Color16ToFloat3(n.nodeVertex.colors[0]);
    //float3 normal = PolarNormalToFloat3(n.nodeVertex.normals[0]);

    //outputBuffer.Append(n.children[0]);
    //
    //vertexBuffer.Append(n.nodeVertex);

    vertexBuffer.Append(nodesBuffer[id.x].nodeVertex);
}