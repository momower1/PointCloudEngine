#include "GroundTruth.hlsl"

//#define DEBUG_SINGLE_QUAD

SamplerState samplerState : register(s0);
Texture2D<float> depthTexture : register(t0);
Texture2D<float4> inputColorTexture : register(t1);
Texture2D<float4> inputNormalTexture : register(t2);
Texture2D<float4> inputPositionTexture : register(t3);
RWTexture2D<float4> outputColorTexture : register(u0);
RWTexture2D<float4> outputNormalTexture : register(u1);
RWTexture2D<float4> outputPositionTexture : register(u2);

cbuffer PullPushConstantBuffer : register(b1)
{
	bool debug;
	bool isPullPhase;
	bool orientedSplat;
	bool texelBlending;
//------------------------------------------------------------------------------ (16 byte boundary)
	int resolutionPullPush;
	int resolutionOutput;
	int resolutionX;
	int resolutionY;
//------------------------------------------------------------------------------ (16 byte boundary)
	float splatSize;
	int pullPushLevel;
	// 8 bytes auto paddding
//------------------------------------------------------------------------------ (16 byte boundary)
};  // Total: 48 bytes with constant buffer packing rules

float2 GetPerpendicularVector(float2 v)
{
	return float2(v.y, -v.x);
}

bool IsInsideQuad(float2 position, float2 quadTopLeft, float2 quadBottomRight, float2 quadLeftNormal, float2 quadTopNormal, float2 quadRightNormal, float2 quadBottomNormal)
{
	bool isInsideTopEdge = dot(position - quadTopLeft, quadTopNormal) > 0;
	bool isInsideLeftEdge = dot(position - quadTopLeft, quadLeftNormal) > 0;
	bool isInsideRightEdge = dot(position - quadBottomRight, quadRightNormal) > 0;
	bool isInsideBottomEdge = dot(position - quadBottomRight, quadBottomNormal) > 0;

	return isInsideTopEdge && isInsideLeftEdge && isInsideRightEdge && isInsideBottomEdge;
}

[numthreads(32, 32, 1)]
void CS(uint3 id : SV_DispatchThreadID)
{
	if (debug)
	{
		outputColorTexture[id.xy] = inputColorTexture.SampleLevel(samplerState, (id.xy + float2(0.5f, 0.5f)) / resolutionOutput, 0);
		return;
	}

#ifdef DEBUG_SINGLE_QUAD
	float3 pointPositionWorld = float3(0, -1, 1);
	float3 pointNormalWorld = float3(0, 1, 0);

	float4 pointPositionNDC = mul(mul(float4(pointPositionWorld, 1), View), Projection);
	pointPositionNDC = pointPositionNDC / pointPositionNDC.w;

	if (pointPositionNDC.z < 0)
	{
		outputColorTexture[id.xy] = float4(1, 0, 0, 1);
		return;
	}

	float3 viewDirection = normalize(pointPositionWorld - cameraPosition);
	float angle = acos(dot(pointNormalWorld, -viewDirection));

	if (angle > (PI / 2))
	{
		outputColorTexture[id.xy] = float4(0.5f, 0, 1, 1);
		return;
	}

	// Construct a quad in world space
	float3 cameraRight = float3(View[0][0], View[1][0], View[2][0]);
	float3 cameraUp = float3(View[0][1], View[1][1], View[2][1]);
	float3 cameraForward = float3(View[0][2], View[1][2], View[2][2]);

	float3 up;
	float3 right;

	// Quad should face in the same direction as the normal or it should be aligned with the camera
	if (orientedSplat)
	{
		up = 0.5f * splatSize * normalize(cross(pointNormalWorld, cameraRight));
		right = 0.5f * splatSize * normalize(cross(pointNormalWorld, up));
	}
	else
	{
		up = 0.5f * splatSize * cameraUp;
		right = 0.5f * splatSize * cameraRight;
	}

	float3 quadCenter = pointPositionWorld;
	float3 quadTopLeft = quadCenter + up - right;
	float3 quadTopRight = quadCenter + up + right;
	float3 quadBottomLeft = quadCenter - up - right;
	float3 quadBottomRight = quadCenter - up + right;

	// Project quad
	float4x4 VP = mul(View, Projection);
	float4 quadCenterNDC = mul(float4(quadCenter, 1), VP);
	float4 quadTopLeftNDC = mul(float4(quadTopLeft, 1), VP);
	float4 quadTopRightNDC = mul(float4(quadTopRight, 1), VP);
	float4 quadBottomLeftNDC = mul(float4(quadBottomLeft, 1), VP);
	float4 quadBottomRightNDC = mul(float4(quadBottomRight, 1), VP);

	// Homogeneous division
	quadCenterNDC /= quadCenterNDC.w;
	quadTopLeftNDC /= quadTopLeftNDC.w;
	quadTopRightNDC /= quadTopRightNDC.w;
	quadBottomLeftNDC /= quadBottomLeftNDC.w;
	quadBottomRightNDC /= quadBottomRightNDC.w;

	// Calculate edge normal vectors (pointing towards quad center)
	float2 quadLeftNormal = GetPerpendicularVector(quadTopLeftNDC.xy - quadBottomLeftNDC.xy);
	float2 quadTopNormal = GetPerpendicularVector(quadTopRightNDC.xy - quadTopLeftNDC.xy);
	float2 quadRightNormal = GetPerpendicularVector(quadBottomRightNDC.xy - quadTopRightNDC.xy);
	float2 quadBottomNormal = GetPerpendicularVector(quadBottomLeftNDC.xy - quadBottomRightNDC.xy);

	uint2 texel = uint2(id.x, id.y);

	// Calculate normalized device coordinates for the four texel corners
	float2 texelTopLeftNDC = resolutionPullPush * (texel / (float)resolutionOutput);
	texelTopLeftNDC = 2 * (texelTopLeftNDC / float2(resolutionX, resolutionY)) - 1;
	float2 texelTopRightNDC = resolutionPullPush * ((texel + uint2(1, 0)) / (float)resolutionOutput);
	texelTopRightNDC = 2 * (texelTopRightNDC / float2(resolutionX, resolutionY)) - 1;
	float2 texelBottomLeftNDC = resolutionPullPush * ((texel + uint2(0, 1)) / (float)resolutionOutput);
	texelBottomLeftNDC = 2 * (texelBottomLeftNDC / float2(resolutionX, resolutionY)) - 1;
	float2 texelBottomRightNDC = resolutionPullPush * ((texel + uint2(1, 1)) / (float)resolutionOutput);
	texelBottomRightNDC = 2 * (texelBottomRightNDC / float2(resolutionX, resolutionY)) - 1;

	// Need to invert y-coordinate for correct coordinate system
	texelTopLeftNDC.y = -texelTopLeftNDC.y;
	texelTopRightNDC.y = -texelTopRightNDC.y;
	texelBottomLeftNDC.y = -texelBottomLeftNDC.y;
	texelBottomRightNDC.y = -texelBottomRightNDC.y;

	// Only color the texel if all of its texel corners are inside the projected quad
	if (IsInsideQuad(texelTopLeftNDC, quadTopLeftNDC.xy, quadBottomRightNDC.xy, quadLeftNormal, quadTopNormal, quadRightNormal, quadBottomNormal)
		&& IsInsideQuad(texelTopRightNDC, quadTopLeftNDC.xy, quadBottomRightNDC.xy, quadLeftNormal, quadTopNormal, quadRightNormal, quadBottomNormal)
		&& IsInsideQuad(texelBottomLeftNDC, quadTopLeftNDC.xy, quadBottomRightNDC.xy, quadLeftNormal, quadTopNormal, quadRightNormal, quadBottomNormal)
		&& IsInsideQuad(texelBottomRightNDC, quadTopLeftNDC.xy, quadBottomRightNDC.xy, quadLeftNormal, quadTopNormal, quadRightNormal, quadBottomNormal))
	{
		outputColorTexture[texel] = float4(0, 1, 0, 1);
	}
	else
	{
		outputColorTexture[texel] = float4(0, 0, 0, 1);
	}

	return;
#endif

	uint2 pixel = id.xy;
	float4 outputColor;
	float4 outputNormal;
	float4 outputPosition;

	if (isPullPhase)
	{
		// Initialize the textures, if there was no point rendered to a pixel, its last texture element component must be zero
		if (pullPushLevel == 0)
		{
			outputColor = float4(0, 0, 0, 0);
			outputNormal = float4(0, 0, 0, 0);
			outputPosition = float4(0, 0, 0, 0);

			if ((pixel.x < resolutionX) && (pixel.y < resolutionY))
			{
				float depth = depthTexture[pixel];

				if (depth < 1.0f)
				{
					// Color of the point
					outputColor = outputColorTexture[pixel];
					outputColor.w = 1.0f;

					// World space normals of the point
					outputNormal = outputNormalTexture[pixel];
					outputNormal.xyz = (2 * outputNormal.xyz) - 1;
					outputNormal.w = 1.0f;

					// Normalized device coordinates of the point
					outputPosition.x = 2 * ((pixel.x + 0.5f) / resolutionX) - 1;
					outputPosition.y = 2 * ((pixel.y + 0.5f) / resolutionY) - 1;
					outputPosition.y = -outputPosition.y;
					outputPosition.z = depth;
					outputPosition.w = 1.0f;
				}
			}
		}
		else
		{
			float3 cameraRight = float3(View[0][0], View[1][0], View[2][0]);
			float3 cameraUp = float3(View[0][1], View[1][1], View[2][1]);
			float3 cameraForward = float3(View[0][2], View[1][2], View[2][2]);

			uint2 texel = uint2(id.x, id.y);

			// Calculate normalized device coordinates for the four texel corners
			float2 texelTopLeftNDC = resolutionPullPush * (texel / (float)resolutionOutput);
			texelTopLeftNDC = 2 * (texelTopLeftNDC / float2(resolutionX, resolutionY)) - 1;
			float2 texelTopRightNDC = resolutionPullPush * ((texel + uint2(1, 0)) / (float)resolutionOutput);
			texelTopRightNDC = 2 * (texelTopRightNDC / float2(resolutionX, resolutionY)) - 1;
			float2 texelBottomLeftNDC = resolutionPullPush * ((texel + uint2(0, 1)) / (float)resolutionOutput);
			texelBottomLeftNDC = 2 * (texelBottomLeftNDC / float2(resolutionX, resolutionY)) - 1;
			float2 texelBottomRightNDC = resolutionPullPush * ((texel + uint2(1, 1)) / (float)resolutionOutput);
			texelBottomRightNDC = 2 * (texelBottomRightNDC / float2(resolutionX, resolutionY)) - 1;

			// Need to invert y-coordinate for correct coordinate system
			texelTopLeftNDC.y = -texelTopLeftNDC.y;
			texelTopRightNDC.y = -texelTopRightNDC.y;
			texelBottomLeftNDC.y = -texelBottomLeftNDC.y;
			texelBottomRightNDC.y = -texelBottomRightNDC.y;

			float smallestDepth = 1.0f;

			outputColor = float4(0, 0, 0, 0);
			outputNormal = float4(0, 0, 0, 0);
			outputPosition = float4(0, 0, 0, 0);

			for (int y = 0; y < 2; y++)
			{
				for (int x = 0; x < 2; x++)
				{
					// Transform quad position and normal into world space
					float4 inputColor = inputColorTexture[2 * pixel + uint2(x, y)];
					float4 inputNormal = inputNormalTexture[2 * pixel + uint2(x, y)];
					float4 inputPosition = inputPositionTexture[2 * pixel + uint2(x, y)];

					float4 quadPositionLocal = mul(inputPosition, WorldViewProjectionInverse);
					quadPositionLocal /= quadPositionLocal.w;

					float3 quadNormalWorld = inputNormal.xyz;
					float3 quadPositionWorld = mul(quadPositionLocal, World).xyz;

					float3 quadUp;
					float3 quadRight;

					// Construct quad that faces in the same direction as the normal or it should be aligned with the camera
					if (orientedSplat)
					{
						quadUp = 0.5f * splatSize * normalize(cross(quadNormalWorld, cameraRight));
						quadRight = 0.5f * splatSize * normalize(cross(quadNormalWorld, quadUp));
					}
					else
					{
						quadUp = 0.5f * splatSize * cameraUp;
						quadRight = 0.5f * splatSize * cameraRight;
					}

					float3 quadCenter = quadPositionWorld;
					float3 quadTopLeft = quadCenter + quadUp - quadRight;
					float3 quadTopRight = quadCenter + quadUp + quadRight;
					float3 quadBottomLeft = quadCenter - quadUp - quadRight;
					float3 quadBottomRight = quadCenter - quadUp + quadRight;

					// Project quad
					float4x4 VP = mul(View, Projection);
					float4 quadCenterNDC = mul(float4(quadCenter, 1), VP);
					float4 quadTopLeftNDC = mul(float4(quadTopLeft, 1), VP);
					float4 quadTopRightNDC = mul(float4(quadTopRight, 1), VP);
					float4 quadBottomLeftNDC = mul(float4(quadBottomLeft, 1), VP);
					float4 quadBottomRightNDC = mul(float4(quadBottomRight, 1), VP);

					// Homogeneous division
					quadCenterNDC /= quadCenterNDC.w;
					quadTopLeftNDC /= quadTopLeftNDC.w;
					quadTopRightNDC /= quadTopRightNDC.w;
					quadBottomLeftNDC /= quadBottomLeftNDC.w;
					quadBottomRightNDC /= quadBottomRightNDC.w;

					// Calculate edge normal vectors (pointing towards quad center)
					float2 quadLeftNormal = GetPerpendicularVector(quadTopLeftNDC.xy - quadBottomLeftNDC.xy);
					float2 quadTopNormal = GetPerpendicularVector(quadTopRightNDC.xy - quadTopLeftNDC.xy);
					float2 quadRightNormal = GetPerpendicularVector(quadBottomRightNDC.xy - quadTopRightNDC.xy);
					float2 quadBottomNormal = GetPerpendicularVector(quadBottomLeftNDC.xy - quadBottomRightNDC.xy);

					if ((inputPosition.w > 0.0f)
						&& (inputPosition.z < smallestDepth)
						&& IsInsideQuad(texelTopLeftNDC, quadTopLeftNDC.xy, quadBottomRightNDC.xy, quadLeftNormal, quadTopNormal, quadRightNormal, quadBottomNormal)
						&& IsInsideQuad(texelTopRightNDC, quadTopLeftNDC.xy, quadBottomRightNDC.xy, quadLeftNormal, quadTopNormal, quadRightNormal, quadBottomNormal)
						&& IsInsideQuad(texelBottomLeftNDC, quadTopLeftNDC.xy, quadBottomRightNDC.xy, quadLeftNormal, quadTopNormal, quadRightNormal, quadBottomNormal)
						&& IsInsideQuad(texelBottomRightNDC, quadTopLeftNDC.xy, quadBottomRightNDC.xy, quadLeftNormal, quadTopNormal, quadRightNormal, quadBottomNormal))
					{
						smallestDepth = inputPosition.z;
						outputColor = inputColor;
						outputNormal = inputNormal;
						outputPosition = inputPosition;
					}
				}
			}
		}
	}
	else
	{
		float2 uv = (pixel + float2(0.5f, 0.5f)) / resolutionOutput;
		float4 inputColor = inputColorTexture.SampleLevel(samplerState, uv, 0);
		float4 inputNormal = inputNormalTexture[pixel / 2];
		float4 inputPosition = inputPositionTexture[pixel / 2];
		
		outputColor = outputColorTexture[pixel];
		outputNormal = outputNormalTexture[pixel];
		outputPosition = outputPositionTexture[pixel];

		// Replace pixels where no point has been rendered to (therefore 4th component is zero) or pixels that are obscured by a closer surface from the higher level pull texture
		if ((inputPosition.w >= 1.0f) && ((outputPosition.w <= 0.0f) || (inputPosition.z < outputPosition.z)))
		{
			outputColor = inputColor;
			outputNormal = inputNormal;
			outputPosition = inputPosition;
		}
	}

	outputColorTexture[pixel] = outputColor;
	outputNormalTexture[pixel] = outputNormal;
	outputPositionTexture[pixel] = outputPosition;
}