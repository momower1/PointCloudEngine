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
    uint normals[3];
    uint colors[3];
    uint weights;
    float size;
};

struct OctreeNode
{
    int children[8];
    OctreeNodeVertex nodeVertex;
};

StructuredBuffer<OctreeNode> nodesBuffer : register(t0);
ConsumeStructuredBuffer<int> inputConsumeBuffer : register(u0);
AppendStructuredBuffer<int> outputAppendBuffer : register(u1);
AppendStructuredBuffer<OctreeNodeVertex> vertexAppendBuffer : register(u2);

[numthreads(1, 1, 1)]
void CS (uint3 id : SV_DispatchThreadID )
{
    int index = inputConsumeBuffer.Consume();

    OctreeNode n = nodesBuffer[index];

    for (int i = 0; i < 8; i++)
    {
        if (n.children[i] >= 0)
        {
            outputAppendBuffer.Append(n.children[i]);
        }
    }

    vertexAppendBuffer.Append(n.nodeVertex);
}