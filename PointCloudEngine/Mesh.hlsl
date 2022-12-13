#include "ShadingMode.hlsli"
#include "LightingConstantBuffer.hlsl"

SamplerState samplerState : register(s0);
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
    float4x4 PreviousWorld;
    //------------------------------------------------------------------------------ (64 byte boundary)
    float4x4 PreviousView;
    //------------------------------------------------------------------------------ (64 byte boundary)
    float4x4 PreviousProjection;
    //------------------------------------------------------------------------------ (64 byte boundary)
    float4x4 PreviousWorldInverseTranspose;
    //------------------------------------------------------------------------------ (64 byte boundary)
    float3 cameraPosition;
    int shadingMode;
    //------------------------------------------------------------------------------ (16 byte boundary)
    int textureLOD;
    // 12 bytes auto padding
};  // Total: 528 bytes with constant buffer packing rules

float3 HUEtoRGB(in float H)
{
    float R = abs(H * 6 - 3) - 1;
    float G = 2 - abs(H * 6 - 2);
    float B = 2 - abs(H * 6 - 4);
    return saturate(float3(R, G, B));
}

float3 HSVtoRGB(in float3 HSV)
{
    float3 RGB = HUEtoRGB(HSV.x);
    return ((RGB - 1) * HSV.y + 1) * HSV.z;
}

float4 FlowToColor(in float2 motion)
{
    float motionColorScale = 0.05f;

    // Visualize the motion vector by mapping its angle to color and its length to saturation (no motion -> black)
    // Up           -> Green
    // Up Right     -> Orange
    // Right        -> Red
    // Down Right   -> Magenta
    // Down         -> Purple
    // Down Left    -> Blue
    // Left         -> Turquoise
    // Up Left      -> Lime
    float saturation = saturate(motionColorScale * length(motion));

    // Calculate signed angle and map to hue component
    float2 direction = normalize(motion);
    float signedAngle = atan2(direction.y, direction.x) - atan2(0.0f, 1.0f);
    float hue = 0.5f + 0.5f * (signedAngle / 3.141592654f);

    // Convert from HSV to RGB
    float3 hsv = float3(hue, saturation, 1.0f);
    float3 color = float3(1.0, 1.0, 1.0) - HSVtoRGB(hsv);

    return float4(color.r, color.g, color.b, 1.0f);
}

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
    float2 textureUV : TEXCOORD;
    float3 normal : NORMAL;
    float3 normalScreen : NORMAL_SCREEN;
    float2 opticalFlow : FLOW;
};

VS_OUTPUT VS(VS_INPUT input)
{
    float3 positionLocal = bufferPositions[input.positionIndex];
    float2 textureUV = bufferTextureCoordinates[input.textureCoordinateIndex];
    float3 normalLocal = bufferNormals[input.normalIndex];

    float4x4 VP = mul(View, Projection);
    float3 positionWorld = mul(float4(positionLocal, 1), World).xyz;
    float4 position = mul(float4(positionWorld, 1), VP);
    float3 normalWorld = normalize(mul(float4(normalLocal, 0), WorldInverseTranspose).xyz);
    float3 normalScreen = normalize(mul(float4(normalWorld, 0), VP).xyz);
    normalScreen.z *= -1;

    float4x4 previousVP = mul(PreviousView, PreviousProjection);
    float3 previousPositionWorld = mul(float4(positionLocal, 1), PreviousWorld).xyz;
    float4 previousPosition = mul(float4(previousPositionWorld, 1), previousVP);

    VS_OUTPUT output;
    output.positionWorld = positionWorld;
    output.position = position;
    output.textureUV = textureUV;
    output.normal = normalWorld;
    output.normalScreen = normalScreen;
    output.opticalFlow = float2(0, 0);

    if (shadingMode == SHADING_MODE_OPTICAL_FLOW_FORWARD)
    {
        // Render previous frame triangles and interpolate the flow towards the next frame
        output.position = previousPosition;
        output.opticalFlow = position.xy - previousPosition.xy;
    }
    else if (shadingMode == SHADING_MODE_OPTICAL_FLOW_BACKWARD)
    {
        // Render current frame triangles and interpolate the flow towards the previous frame triangles
        output.opticalFlow = previousPosition.xy - position.xy;
    }

    return output;
}

float4 PS(VS_OUTPUT input) : SV_TARGET
{
    input.normal = normalize(input.normal);
    input.normalScreen = normalize(input.normalScreen);

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
        case SHADING_MODE_OPTICAL_FLOW_BACKWARD:
        {
            return float4(input.opticalFlow.x, input.opticalFlow.y, 0, 1);
            return FlowToColor(input.opticalFlow);
        }
    }

    return float4(1, 0, 0, 1);
}