#include "GroundTruth.hlsl"

Texture2D<float> depthTexture : register(t0);
Texture2D<float4> inputColorTexture : register(t1);
Texture2D<float4> inputImportanceTexture : register(t2);
RWTexture2D<float4> outputColorTexture : register(u0);
RWTexture2D<float4> outputImportanceTexture : register(u1);
SamplerState samplerState : register(s0);

cbuffer PullPushConstantBuffer : register(b1)
{
	int resolutionX;
	int resolutionY;
	int resolutionOutput;
	int pullPushLevel;
//------------------------------------------------------------------------------ (16 byte boundary)
	float depthBias;
	float importanceScale;
	float importanceExponent;
	bool isPullPhase;
//------------------------------------------------------------------------------ (16 byte boundary)
	bool drawImportance;
// 12 bytes auto paddding
};  // Total: 48 bytes with constant buffer packing rules

[numthreads(32, 32, 1)]
void CS(uint3 id : SV_DispatchThreadID)
{
	uint2 pixel = id.xy;
	float4 outputColor;
	float4 outputImportance;

	if (isPullPhase)
	{
		// Initialize the texture such that each pixel contains the point color in the RGB and its depth in the A component
		// If there was no point rendered to a pixel, its depth must be 1
		if (pullPushLevel == 0)
		{
			// NDC coordinates
			outputImportance = float4(2 * ((pixel.x + 0.5f) / resolutionX) - 1, 2 * ((pixel.y + 0.5f) / resolutionY) - 1, 1.0f, 1.0f);

			if ((pixel.x < resolutionX) && (pixel.y < resolutionY))
			{
				outputColor = outputColorTexture[pixel];
				outputColor.w = depthTexture[pixel];

				outputImportance.z = depthTexture[pixel];
			}
			else
			{
				outputColor = float4(0, 0, 0, 1);
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

			outputColor = float4(0, 0, 0, 1);
			outputImportance = float4(0, 0, 1, 1);

			for (int y = 0; y < 2; y++)
			{
				for (int x = 0; x < 2; x++)
				{
					float4 inputColor = inputColorTexture[2 * pixel + uint2(x, y)];
					float4 inputImportance = inputImportanceTexture[2 * pixel + uint2(x, y)];

					float4 inputPosition = mul(inputImportance, WorldViewProjectionInverse);
					inputPosition = inputPosition / inputPosition.w;

					float distanceInputToCamera = distance(cameraPosition, inputPosition);
					float splatSize = depthBias;
					float projectedSplatSizeInput = splatSize / ((2.0f * tan(fovAngleY / 2.0f)) * distanceInputToCamera);

					float distanceTopLeft = distance(outputTexelTopLeft, inputImportance.xy);
					float distanceTopRight = distance(outputTexelTopRight, inputImportance.xy);
					float distanceBottomLeft = distance(outputTexelBottomLeft, inputImportance.xy);
					float distanceBottomRight = distance(outputTexelBottomRight, inputImportance.xy);

					if ((inputImportance.z < smallestDepth) && (distanceTopLeft < projectedSplatSizeInput) && (distanceTopRight < projectedSplatSizeInput) && (distanceBottomLeft < projectedSplatSizeInput) && (distanceBottomRight < projectedSplatSizeInput))
					{
						smallestDepth = inputImportance.z;
						outputColor = inputColor;
						outputImportance = inputImportance;
					}
				}
			}
		}
	}
	else
	{
		float2 uv = (pixel + float2(0.5f, 0.5f)) / resolutionOutput;
		float4 inputColor = inputColorTexture.SampleLevel(samplerState, uv, 0);
		float4 inputImportance = inputImportanceTexture.SampleLevel(samplerState, uv, 0);
		
		outputColor = outputColorTexture[pixel];
		outputImportance = outputImportanceTexture[pixel];

		// Only replace pixels where no point has been rendered to (therefore depth is 1) with values from the higher level pull texture
		if ((outputImportance.z >= 1.0f) || (inputImportance.z < outputImportance.z))
		{
			outputColor = inputColor;
			outputImportance = inputImportance;
		}
	}

	outputColorTexture[pixel] = outputColor;
	outputImportanceTexture[pixel] = outputImportance;
}