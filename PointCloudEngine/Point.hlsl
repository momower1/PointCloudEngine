#include "GroundTruth.hlsl"
#include "ShadingMode.hlsli"
#include "LightingConstantBuffer.hlsl"

struct GS_POINT_OUTPUT
{
	float4 position : SV_POSITION;
	float3 positionWorld : POSITION1;
	float4 positionClip : POSITION2;
	float4 positionClipPrevious : POSITION3;
	float3 normalScreen : NORMAL_SCREEN;
	float3 normal : NORMAL;
	float3 color : COLOR;
};

[maxvertexcount(1)]
void GS(point VS_OUTPUT input[1], inout PointStream<GS_POINT_OUTPUT> output)
{
	// Discard points that have a normal facing away from the camera
	if (backfaceCulling)
	{
		float3 viewDirection = normalize(input[0].position - cameraPosition);
		float angle = acos(dot(input[0].normal, -viewDirection));

		if (angle > (PI / 2))
		{
			return;
		}
	}

	float4x4 VP = mul(View, Projection);
	float4x4 PreviousVP = mul(PreviousView, PreviousProjection);

	GS_POINT_OUTPUT element;
	element.position = mul(float4(input[0].position, 1), VP);
	element.positionWorld = input[0].position;
	element.positionClip = element.position;
	element.positionClipPrevious = mul(float4(input[0].positionPrevious, 1), PreviousVP);
	element.normalScreen = normalize(mul(float4(input[0].normal, 0), VP).xyz);
	element.normalScreen.z *= -1;
	element.normal = input[0].normal;
	element.color = input[0].color;

	// Render previous frame point instead of current frame if performing backward optical flow
	if (shadingMode == SHADING_MODE_OPTICAL_FLOW_BACKWARD)
	{
		element.position = element.positionClipPrevious;
	}

	output.Append(element);
}

float4 PS(GS_POINT_OUTPUT input) : SV_TARGET
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
			if (useLighting)
			{
				return float4(PhongLighting(cameraPosition, input.positionWorld, input.normal, input.color), 1);
			}

			return float4(input.color, 1);
		}
		case SHADING_MODE_DEPTH:
		{
			return float4(input.position.z, input.position.z, input.position.z, 1);
		}
		case SHADING_MODE_NORMAL:
		{
			return float4(0.5f * (input.normal + 1), 1);
		}
		case SHADING_MODE_NORMAL_SCREEN:
		{
			return float4(0.5f * (input.normalScreen + 1), 1);
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