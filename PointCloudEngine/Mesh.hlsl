#include "ShadingMode.hlsli"
#include "GroundTruthConstantBuffer.hlsli"
#include "LightingConstantBuffer.hlsl"

SamplerState samplerState : register(s0);
Texture2D<float4> textureAlbedo : register(t0);
StructuredBuffer<float3> bufferPositions : register(t1);
StructuredBuffer<float2> bufferTextureCoordinates : register(t2);
StructuredBuffer<float3> bufferNormals : register(t3);

struct VS_INPUT
{
    uint positionIndex : POSITION_INDEX;
    uint textureCoordinateIndex : TEXCOORD_INDEX;
    uint normalIndex : NORMAL_INDEX;
};

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float3 positionWorld : POSITION;
    float4 positionClip : POSITION1;
    float4 positionClipPrevious : POSITION2;
    float2 textureUV : TEXCOORD;
    float3 normal : NORMAL;
    float3 normalScreen : NORMAL_SCREEN;
};

VS_OUTPUT VS(VS_INPUT input)
{
    float3 positionLocal = bufferPositions[input.positionIndex];
    float2 textureUV = bufferTextureCoordinates[input.textureCoordinateIndex];
    float3 normalLocal = bufferNormals[input.normalIndex];

    float4x4 VP = mul(View, Projection);
    float4x4 PreviousVP = mul(PreviousView, PreviousProjection);

    VS_OUTPUT output;
    output.positionWorld = mul(float4(positionLocal, 1), World).xyz;
    output.position = mul(float4(output.positionWorld, 1), VP);
    output.positionClip = output.position;
    output.positionClipPrevious = mul(float4(positionLocal, 1), PreviousWorld);
    output.positionClipPrevious = mul(output.positionClipPrevious, PreviousVP);
    output.textureUV = textureUV;
    output.normal = normalize(mul(float4(normalLocal, 0), WorldInverseTranspose).xyz);
    output.normalScreen = normalize(mul(float4(output.normal, 0), VP).xyz);
    output.normalScreen.z *= -1;

    // Render previous frame triangles instead of current frame if performing backward optical flow
    if (shadingMode == SHADING_MODE_OPTICAL_FLOW_BACKWARD)
    {
        output.position = output.positionClipPrevious;
    }

    return output;
}

float4 PS(VS_OUTPUT input) : SV_TARGET
{
    input.normal = normalize(input.normal);
    input.normalScreen = normalize(input.normalScreen);

    // Compute pixel positions for optical flow calculation (motion vectors in pixel space)
    float4 previousPositionNDC = input.positionClipPrevious / input.positionClipPrevious.w;
    previousPositionNDC.y *= -1;

    float4 positionNDC = input.positionClip / input.positionClip.w;
    positionNDC.y *= -1;

    float2 previousPixel = (previousPositionNDC.xy + 1.0f) / 2.0f;
    previousPixel *= float2(resolutionX, resolutionY);

    float2 pixel = (positionNDC.xy + 1.0f) / 2.0f;
    pixel *= float2(resolutionX, resolutionY);

    switch (shadingMode)
    {
        case SHADING_MODE_COLOR:
        {
            float3 albedo = textureAlbedo.SampleLevel(samplerState, input.textureUV, textureLOD).rgb;

            if (useLighting)
            {
                return float4(PhongLighting(cameraPosition, input.positionWorld, input.normal, albedo), 1);
            }
            else
            {
                return float4(albedo, 1);
            }
        }
        case SHADING_MODE_DEPTH:
        {
            return float4(input.position.z, input.position.z, input.position.z, 1);
        }
        case SHADING_MODE_NORMAL:
        {
            return float4(0.5f * (input.normal + 1.0f), 1);
        }
        case SHADING_MODE_NORMAL_SCREEN:
        {
            return float4(0.5f * (input.normalScreen + 1.0f), 1);
        }
        case SHADING_MODE_OPTICAL_FLOW_FORWARD:
        {
            float2 flow = previousPixel - pixel;
            return float4(flow.x, flow.y, 0, 1);
        }
        case SHADING_MODE_OPTICAL_FLOW_BACKWARD:
        {
            float2 flow = pixel - previousPixel;
            return float4(flow.x, flow.y, 0, 1);
        }
    }

    return float4(1, 0, 0, 1);
}