#include "LightingConstantBuffer.hlsl"

Texture2D<float4> textureAlbedo : register(t0);
StructuredBuffer<float3> bufferPositions : register(t1);
StructuredBuffer<float2> bufferTextureCoordinates : register(t2);
StructuredBuffer<float3> bufferNormals : register(t3);

cbuffer MeshRendererConstantBuffer : register(b0)
{
    float4x4 World;
    //------------------------------------------------------------------------------ (64 byte boundary)
    float4x4 View;
    //------------------------------------------------------------------------------ (64 byte boundary)
    float4x4 Projection;
    //------------------------------------------------------------------------------ (64 byte boundary)
    float4x4 WorldInverseTranspose;
    //------------------------------------------------------------------------------ (64 byte boundary)
    float3 cameraPosition;
    // 4 bytes auto padding
};  // Total: 272 bytes with constant buffer packing rules

SamplerState LinearSampler
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Clamp;
    AddressV = Clamp;
};

struct VS_INPUT
{
    uint positionIndex : POSITION;
    uint textureCoordinateIndex : TEXCOORD;
    uint normalIndex : NORMAL;
};

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float3 positionWorld : POSITION;
    float2 textureUV : TEXCOORD;
    float3 normal : NORMAL;
};

VS_OUTPUT VS(VS_INPUT input)
{
    VS_OUTPUT output;
    output.positionWorld = mul(float4(bufferPositions[input.positionIndex], 1), World);
    output.position = mul(output.positionWorld, View);
    output.position = mul(output.position, Projection);
    output.normal = normalize(mul(float4(bufferNormals[input.normalIndex], 0), WorldInverseTranspose)).xyz;
    output.textureUV = bufferTextureCoordinates[input.textureCoordinateIndex];

    return output;
}

float4 PS(VS_OUTPUT input) : SV_TARGET
{
    float3 albedo = textureAlbedo.Sample(LinearSampler, input.textureUV).rgb;
    float3 color = PhongLighting(cameraPosition, input.positionWorld, input.normal, albedo);

    return float4(color, 1);
}