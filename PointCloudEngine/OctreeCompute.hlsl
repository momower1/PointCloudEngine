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

AppendStructuredBuffer<OctreeNode> one : register(u0);

[numthreads(1, 1, 1)]
void CS (uint3 id : SV_DispatchThreadID )
{
    // TODO: Remove
    OctreeNode n;

    float3 color = Color16ToFloat3(n.nodeVertex.colors[0]);
    float3 normal = PolarNormalToFloat3(n.nodeVertex.normals[0]);

    //one.Append(n);
}