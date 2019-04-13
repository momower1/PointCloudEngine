#include "Common.hlsl"

cbuffer ComputeShaderConstantBuffer : register(b0)
{
    float3 localCameraPosition;
    float fovAngleY;
//------------------------------------------------------------------------------ (16 byte boundary)
    float splatSize;
//  12 bytes auto padding
};  // Total: 32 bytes with constant buffer packing rules

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

    OctreeNode node = nodesBuffer[index];

    float distanceToCamera = distance(localCameraPosition, node.nodeVertex.position);
    float requiredSplatSize = splatSize * (2.0f * tan(fovAngleY / 2.0f)) * distanceToCamera;
    bool isLeafNode = true;

    for (int i = 0; i < 8; i++)
    {
        if (node.children[i] >= 0)
        {
            isLeafNode = false;
            break;
        }
    }

    if (node.nodeVertex.size < requiredSplatSize || isLeafNode)
    {
        if (node.nodeVertex.size < epsilon)
        {
            OctreeNodeVertex tmp = node.nodeVertex;
            tmp.size = requiredSplatSize;

            vertexAppendBuffer.Append(tmp);
        }
        else
        {
            vertexAppendBuffer.Append(node.nodeVertex);
        }
    }
    else
    {
        for (int i = 0; i < 8; i++)
        {
            if (node.children[i] >= 0)
            {
                outputAppendBuffer.Append(node.children[i]);
            }
        }
    }
}