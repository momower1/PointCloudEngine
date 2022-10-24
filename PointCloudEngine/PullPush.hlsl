#include "GroundTruth.hlsl"

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
	int resolutionX;
	int resolutionY;
//------------------------------------------------------------------------------ (16 byte boundary)
	int resolutionOutput;
	float splatSize;
	int pullPushLevel;
	// 4 bytes auto paddding
//------------------------------------------------------------------------------ (16 byte boundary)
};  // Total: 32 bytes with constant buffer packing rules

bool IsInsideQuad(float2 position, float2 edgeTopPosition, float2 edgeTopNormal, float2 edgeLeftPosition, float2 edgeLeftNormal, float2 edgeRightPosition, float2 edgeRightNormal, float2 edgeBottomPosition, float2 edgeBottomNormal)
{
	bool isInsideTopEdge = dot(position - edgeTopPosition, edgeTopNormal) > 0;
	bool isInsideLeftEdge = dot(position - edgeLeftPosition, edgeLeftNormal) > 0;
	bool isInsideRightEdge = dot(position - edgeRightPosition, edgeRightNormal) > 0;
	bool isInsideBottomEdge = dot(position - edgeBottomPosition, edgeBottomNormal) > 0;

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

/////////////////////////////////////////////////////
// TESTING

	float3 pointNormalLocal = float3(0, 0, -1);
	float3 pointPositionLocal = float3(0, 0, 1);
	float3 pointPositionWorld = mul(float4(pointPositionLocal, 1), World).xyz;
	float4 pointPositionNDC = mul(mul(float4(pointPositionWorld, 1), View), Projection);
	pointPositionNDC = pointPositionNDC / pointPositionNDC.w;

	if (pointPositionNDC.z < 0)
	{
		outputColorTexture[id.xy] = float4(1, 0, 0, 1);
		return;
	}

	float3 pointNormalWorld = normalize(mul(float4(pointNormalLocal, 0), WorldInverseTranspose).xyz);

	float3 viewDirection = normalize(pointPositionWorld - cameraPosition);
	float angle = acos(dot(pointNormalWorld, -viewDirection));

	if (angle > (PI / 2))
	{
		outputColorTexture[id.xy] = float4(0.5f, 0, 1, 1);
		return;
	}

	// Construct quad in world space
	float3 cameraRight = float3(View[0][0], View[1][0], View[2][0]);
	float3 cameraUp = float3(View[0][1], View[1][1], View[2][1]);
	float3 cameraForward = float3(View[0][2], View[1][2], View[2][2]);

	// Billboard should face in the same direction as the normal
	float3 up = 0.5f * splatSize * normalize(cross(pointNormalWorld, cameraRight));
	float3 right = 0.5f * splatSize * normalize(cross(pointNormalWorld, up));

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
	float2 quadEdgeTop = quadTopRightNDC - quadTopLeftNDC.xy;
	float2 quadEdgeTopNormal = normalize(quadCenterNDC.xy - (quadTopLeftNDC.xy + dot(quadCenterNDC.xy - quadTopLeftNDC, quadEdgeTop) * quadEdgeTop));
	float2 quadEdgeLeft = quadBottomLeftNDC - quadTopLeftNDC.xy;
	float2 quadEdgeLeftNormal = normalize(quadCenterNDC.xy - (quadTopLeftNDC.xy + dot(quadCenterNDC.xy - quadTopLeftNDC, quadEdgeLeft) * quadEdgeLeft));
	float2 quadEdgeRight = quadBottomRightNDC - quadTopRightNDC.xy;
	float2 quadEdgeRightNormal = normalize(quadCenterNDC.xy - (quadTopRightNDC.xy + dot(quadCenterNDC.xy - quadTopRightNDC, quadEdgeRight) * quadEdgeRight));
	float2 quadEdgeBottom = quadBottomRightNDC - quadBottomLeftNDC.xy;
	float2 quadEdgeBottomNormal = normalize(quadCenterNDC.xy - (quadBottomLeftNDC.xy + dot(quadCenterNDC.xy - quadBottomLeftNDC, quadEdgeBottom) * quadEdgeBottom));

	uint2 actualId = uint2(id.x, resolutionY - id.y - 1);
	float2 actualIdNDC = (2 * (actualId / float2(resolutionX, resolutionY))) - 1;

	if (distance(actualIdNDC, quadTopLeftNDC.xy) < 0.01f)
	{
		outputColorTexture[actualId] = float4(0, 0, 1, 1);
	}
	else if (distance(actualIdNDC, quadTopRightNDC.xy) < 0.01f)
	{
		outputColorTexture[actualId] = float4(0, 0, 1, 1);
	}
	else if (distance(actualIdNDC, quadBottomLeftNDC.xy) < 0.01f)
	{
		outputColorTexture[actualId] = float4(0, 0, 1, 1);
	}
	else if (distance(actualIdNDC, quadBottomRightNDC.xy) < 0.01f)
	{
		outputColorTexture[actualId] = float4(0, 0, 1, 1);
	}
	else if (distance(actualIdNDC, quadCenterNDC.xy) < 0.01f)
	{
		outputColorTexture[actualId] = float4(0, 0, 1, 1);
	}
	else
	{
		float2 edgeLeft = quadBottomLeftNDC.xy - quadTopLeftNDC.xy;
		float2 edgeLeftN = normalize(cross(float3(0, 0, 1), float3(edgeLeft, 0)));

		float2 edgeRight = quadBottomRightNDC.xy - quadTopRightNDC.xy;
		float2 edgeRightN = normalize(cross(float3(0, 0, 1), float3(edgeRight, 0)));

		float2 edgeTop = quadTopRightNDC.xy - quadTopLeftNDC.xy;
		float2 edgeTopN = normalize(cross(float3(0, 0, 1), float3(edgeTop, 0)));

		float2 edgeBottom = quadBottomRightNDC.xy - quadBottomLeftNDC.xy;
		float2 edgeBottomN = normalize(cross(float3(0, 0, 1), float3(edgeBottom, 0)));

		if ((dot(actualIdNDC - quadTopLeftNDC, edgeLeftN) > 0)
			&& (dot(actualIdNDC - quadTopLeftNDC, edgeTopN) < 0)
			&& (dot(actualIdNDC - quadBottomRightNDC, edgeRightN) < 0)
			&& (dot(actualIdNDC - quadBottomRightNDC, edgeBottomN) > 0))
		{
			outputColorTexture[actualId] = float4(0, 1, 0, 1);
		}
		else
		{
			outputColorTexture[actualId] = float4(0, 0, 0, 1);
		}
	}

	return;

	int resolutionFull = pow(2, ceil(log2(max(resolutionX, resolutionY))));

	float2 absoluteTopLeft = resolutionFull * (id.xy / (float)resolutionOutput);
	float2 ndcTopLeft = 2 * (absoluteTopLeft / float2(resolutionX, resolutionY)) - 1;

	float2 absoluteTopRight = resolutionFull * ((id.xy + uint2(1, 0)) / (float)resolutionOutput);
	float2 ndcTopRight = 2 * (absoluteTopRight / float2(resolutionX, resolutionY)) - 1;

	float2 absoluteBottomLeft = resolutionFull * ((id.xy + uint2(0, 1)) / (float)resolutionOutput);
	float2 ndcBottomLeft = 2 * (absoluteBottomLeft / float2(resolutionX, resolutionY)) - 1;

	float2 absoluteBottomRight = resolutionFull * ((id.xy + uint2(1, 1)) / (float)resolutionOutput);
	float2 ndcBottomRight = 2 * (absoluteBottomRight / float2(resolutionX, resolutionY)) - 1;

	if (IsInsideQuad(ndcTopLeft.xy, quadTopLeftNDC.xy, quadEdgeTopNormal, quadTopLeftNDC.xy, quadEdgeLeftNormal, quadTopRightNDC.xy, quadEdgeRightNormal, quadBottomLeftNDC.xy, quadEdgeBottomNormal))
	{
		outputColorTexture[id.xy] = float4(0, 0, 1, 1);
	}
	else
	{
		outputColorTexture[id.xy] = float4(0, 0, 0, 1);
	}

	return;

// TESTING
////////////////////////////////////////////////////

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
					outputPosition.z = depth;
					outputPosition.w = 1.0f;
				}
			}
		}
		else
		{
			int resolutionFull = pow(2, ceil(log2(max(resolutionX, resolutionY))));

			// Compute NDC coordinates of the 4 corners of the output texel
			float2 outputTexelTopLeft = resolutionFull * (pixel / (float)resolutionOutput);
			outputTexelTopLeft /= float2(resolutionX, resolutionY);
			outputTexelTopLeft = (2 * outputTexelTopLeft) - 1;

			float2 outputTexelTopRight = resolutionFull * ((pixel + uint2(1, 0)) / (float)resolutionOutput);
			outputTexelTopRight /= float2(resolutionX, resolutionY);
			outputTexelTopRight = (2 * outputTexelTopRight) - 1;

			float2 outputTexelBottomLeft = resolutionFull * ((pixel + uint2(0, 1)) / (float)resolutionOutput);
			outputTexelBottomLeft /= float2(resolutionX, resolutionY);
			outputTexelBottomLeft = (2 * outputTexelBottomLeft) - 1;

			float2 outputTexelBottomRight = resolutionFull * ((pixel + uint2(1, 1)) / (float)resolutionOutput);
			outputTexelBottomRight /= float2(resolutionX, resolutionY);
			outputTexelBottomRight = (2 * outputTexelBottomRight) - 1;

			float4 cameraPosition = float4(0, 0, 0, 1);
			cameraPosition = mul(cameraPosition, WorldViewProjectionInverse);
			cameraPosition = cameraPosition / cameraPosition.w;

			float smallestDepth = 1.0f;

			outputColor = float4(0, 0, 0, 0);
			outputNormal = float4(0, 0, 0, 0);
			outputPosition = float4(0, 0, 0, 0);

			for (int y = 0; y < 2; y++)
			{
				for (int x = 0; x < 2; x++)
				{
					float4 inputColor = inputColorTexture[2 * pixel + uint2(x, y)];
					float4 inputPosition = inputPositionTexture[2 * pixel + uint2(x, y)];

					float4 inputPositionLocal = mul(inputPosition, WorldViewProjectionInverse);
					inputPositionLocal = inputPositionLocal / inputPositionLocal.w;

					float distanceInputToCamera = distance(cameraPosition, inputPositionLocal);
					float projectedSplatSizeInput = splatSize / ((2.0f * tan(fovAngleY / 2.0f)) * distanceInputToCamera);

					float distanceTopLeft = distance(outputTexelTopLeft, inputPosition.xy);
					float distanceTopRight = distance(outputTexelTopRight, inputPosition.xy);
					float distanceBottomLeft = distance(outputTexelBottomLeft, inputPosition.xy);
					float distanceBottomRight = distance(outputTexelBottomRight, inputPosition.xy);

					if ((inputPosition.w > 0.0f) && (inputPosition.z < smallestDepth) && (distanceTopLeft < projectedSplatSizeInput) && (distanceTopRight < projectedSplatSizeInput) && (distanceBottomLeft < projectedSplatSizeInput) && (distanceBottomRight < projectedSplatSizeInput))
					{
						smallestDepth = inputPosition.z;
						outputColor = inputColor;
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
		float4 inputPosition = inputPositionTexture[pixel / 2];
		
		outputColor = outputColorTexture[pixel];
		outputPosition = outputPositionTexture[pixel];

		// Replace pixels where no point has been rendered to (therefore 4th component is zero) or pixels that are obscured by a closer surface from the higher level pull texture
		if ((inputPosition.w >= 1.0f) && ((outputPosition.w <= 0.0f) || (inputPosition.z < outputPosition.z)))
		{
			outputColor = inputColor;
			outputPosition = inputPosition;
		}
	}

	outputColorTexture[pixel] = outputColor;
	outputPositionTexture[pixel] = outputPosition;
}