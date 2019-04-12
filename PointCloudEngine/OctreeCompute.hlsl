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
    half weights[6];
    float size;
};

struct OctreeNode
{
    int children[8];
    OctreeNodeVertex nodeVertex;
};

StructuredBuffer<OctreeNode> nodes : register(t0);
ConsumeStructuredBuffer<int> input : register(u0);
AppendStructuredBuffer<int> output : register(u1);
AppendStructuredBuffer<OctreeNodeVertex> vertices : register(u2);

[numthreads(1, 1, 1)]
void CS (uint3 id : SV_DispatchThreadID )
{
    int index = input.Consume();

    // TODO: Remove
    OctreeNode n = nodes[index];

    float3 color = Color16ToFloat3(n.nodeVertex.colors[0]);
    float3 normal = PolarNormalToFloat3(n.nodeVertex.normals[0]);

    output.Append(n.children[0]);
    
    vertices.Append(n.nodeVertex);
}