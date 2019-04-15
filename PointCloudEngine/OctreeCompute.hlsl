#include "Octree.hlsl"

cbuffer ComputeShaderConstantBuffer : register(b0)
{
    float3 localCameraPosition;
    float fovAngleY;
//------------------------------------------------------------------------------ (16 byte boundary)
    float splatSize;
    uint inputCount;
//  8 bytes auto padding
};  // Total: 32 bytes with constant buffer packing rules

StructuredBuffer<OctreeNode> nodesBuffer : register(t0);
ConsumeStructuredBuffer<uint> inputConsumeBuffer : register(u0);
AppendStructuredBuffer<uint> outputAppendBuffer : register(u1);
AppendStructuredBuffer<uint> vertexAppendBuffer : register(u2);

[numthreads(1024, 1, 1)]
void CS (uint3 id : SV_DispatchThreadID)
{
    // Make sure that this thread is working on valid data
    if (id.x < inputCount)
    {
        // Get the index of the node that this thread has to check
        uint index = inputConsumeBuffer.Consume();

        OctreeNode node = nodesBuffer[index];

        // Calculate required splat size
        float distanceToCamera = distance(localCameraPosition, node.nodeVertex.position);
        float requiredSplatSize = splatSize * (2.0f * tan(fovAngleY / 2.0f)) * distanceToCamera;
        bool isLeafNode = true;

        for (int i = 0; i < 8; i++)
        {
            if (node.children[i] != UINT_MAX)
            {
                isLeafNode = false;
                break;
            }
        }

        // Check against required splat size and draw this vertex if it is smaller
        if (node.nodeVertex.size < requiredSplatSize || isLeafNode)
        {
            vertexAppendBuffer.Append(index);
        }
        else
        {
            // Check all the children in the next compute shader iteration
            for (int i = 0; i < 8; i++)
            {
                if (node.children[i] != UINT_MAX)
                {
                    outputAppendBuffer.Append(node.children[i]);
                }
            }
        }
    }
}