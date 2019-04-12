cbuffer ComputeShaderConstantBuffer : register(b0)
{
    float3 localCameraPosition;
    float splatSize;
//------------------------------------------------------------------------------ (16 byte boundary)
};  // Total: 16 bytes with constant buffer packing rules

[numthreads(1, 1, 1)]
void CS (uint3 id : SV_DispatchThreadID )
{
    // TODO
}