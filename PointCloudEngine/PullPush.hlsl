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
			float smallestDepth = 1.0f;

			for (int y = 0; y < 2; y++)
			{
				for (int x = 0; x < 2; x++)
				{
					float4 inputColor = inputColorTexture[2 * pixel + uint2(x, y)];
					float4 inputImportance = inputImportanceTexture[2 * pixel + uint2(x, y)];

					if (inputColor.w <= smallestDepth)
					{
						smallestDepth = inputColor.w;
						outputColor = inputColor;
						outputImportance = inputImportance;
					}
				}
			}

			//// Instead of closest pixel depthwise, choose closest pixel to the center position
			//float smallestDistanceInScreenSpace = 200.0f;

			//for (int y = 0; y < 2; y++)
			//{
			//	for (int x = 0; x < 2; x++)
			//	{
			//		float2 positionScreen = (pixel + float2(0.5f, 0.5f)) / resolutionOutput;
			//		positionScreen.x = positionScreen.x * pow(2, int(log2(max(resolutionX, resolutionY))));
			//		positionScreen.y = positionScreen.y * pow(2, int(log2(max(resolutionX, resolutionY))));
			//		positionScreen.x /= resolutionX;
			//		positionScreen.y /= resolutionY;
			//		positionScreen.x = positionScreen.x * 2 - 1;
			//		positionScreen.y = positionScreen.y * 2 - 1;

			//		float4 inputColor = inputColorTexture[2 * pixel + uint2(x, y)];
			//		float4 inputImportance = inputImportanceTexture[2 * pixel + uint2(x, y)];

			//		if (distance(inputImportance.xy, positionScreen.xy) < smallestDistanceInScreenSpace)
			//		{
			//			smallestDistanceInScreenSpace = inputColor.w;
			//			outputColor = inputColor;
			//			outputImportance = inputImportance;
			//		}
			//	}
			//}
		}
	}
	else
	{
		float2 uv = (pixel + float2(0.5f, 0.5f)) / resolutionOutput;
		float4 inputColor = inputColorTexture.SampleLevel(samplerState, uv, 0);
		float4 inputImportance = inputImportanceTexture.SampleLevel(samplerState, uv, 0);
		
		outputColor = outputColorTexture[pixel];
		outputImportance = outputImportanceTexture[pixel];

		// Construct normalized device coordinates for the points in range [-1, 1]
		float4 inputPositionScreen = inputImportance;
		float4 outputPositionScreen = outputImportance;

		//outputPositionScreen.x = min(resolutionX, uv.x * resolutionOutput);
		//outputPositionScreen.y = min(resolutionY, uv.y * resolutionOutput);
		//outputPositionScreen.x /= resolutionX;
		//outputPositionScreen.y /= resolutionY;
		//outputPositionScreen.x = outputPositionScreen.x * 2 - 1;
		//outputPositionScreen.y = outputPositionScreen.y * 2 - 1;

		//outputColorTexture[pixel] = float4(outputPositionScreen.x, outputPositionScreen.y, 0, 1);
		//outputColorTexture[pixel] = float4(inputPositionScreen.x, inputPositionScreen.y, 0, 1);
		//return;


		//if (outputPositionScreen.z >= 1.0f)
		//{
		//	outputColor = inputColor;
		//	outputImportance = inputImportance;
		//}
		{
			// Transform back into object space because comparing the depth values directly doesn't work well since they are not distributed linearely
			float4 inputPosition = mul(inputPositionScreen, WorldViewProjectionInverse);
			inputPosition = inputPosition / inputPosition.w;

			float4 outputPosition = mul(outputPositionScreen, WorldViewProjectionInverse);
			outputPosition = outputPosition / outputPosition.w;

			float4 cameraPosition = float4(0, 0, 0, 1);
			cameraPosition = mul(cameraPosition, WorldViewProjectionInverse);
			cameraPosition = cameraPosition / cameraPosition.w;

			float distanceInputToCamera = distance(cameraPosition, inputPosition);
			float distanceOutputToCamera = distance(cameraPosition, outputPosition);
			float splatSize = depthBias;
			float projectedSplatSizeInput = splatSize * (2.0f * tan(fovAngleY / 2.0f)) * distanceInputToCamera;
			float projectedSplatSizeOutput = splatSize * (2.0f * tan(fovAngleY / 2.0f)) * distanceOutputToCamera;

			projectedSplatSizeInput = splatSize / ((2.0f * tan(fovAngleY / 2.0f)) * distanceInputToCamera);

			// Ignore pixels that "shine through" closer surfaces
			if (distance(inputPositionScreen.xy, outputPositionScreen.xy) < projectedSplatSizeInput)
			{
				outputColor = inputColor;
				outputImportance = inputImportance;
			}
		}


		//if (true)
		//{
		//	outputColor.r = (outputImportance.x + 1) / 2;
		//	outputColor.g = (outputImportance.y + 1) / 2;
		//	outputColor.b = 0;
		//	outputColor.a = 1;

		//	if (outputImportance.z >= 1.0f)
		//	{
		//		outputColor = float4(0, 0, 0, 1);
		//	}
		//}
		// Only replace pixels where no point has been rendered to (therefore depth is 1) with values from the higher level pull texture
		//if (outputColor.w >= 1.0f)
		//{
		//	outputColor = inputColor;
		//	outputImportance = inputImportance;
		//}
		//// Ignore pixels that shine through the closest surface and replace them
		//else
		//{
		//	// Construct normalized device coordinates for the points in range [-1, 1]
		//	float4 inputPositionScreen = inputImportance;
		//	float4 outputPositionScreen = outputImportance;

		//	// Transform back into object space because comparing the depth values directly doesn't work well since they are not distributed linearely
		//	float4 inputPosition = mul(inputPositionScreen, WorldViewProjectionInverse);
		//	inputPosition = inputPosition / inputPosition.w;

		//	float4 outputPosition = mul(outputPositionScreen, WorldViewProjectionInverse);
		//	outputPosition = outputPosition / outputPosition.w;

		//	float4 cameraPosition = float4(0, 0, 0, 1);
		//	cameraPosition = mul(cameraPosition, WorldViewProjectionInverse);
		//	cameraPosition = cameraPosition / cameraPosition.w;

		//	float distanceInputToCamera = distance(cameraPosition, inputPosition);
		//	float distanceOutputToCamera = distance(cameraPosition, outputPosition);
		//	float splatSize = depthBias;
		//	float projectedSplatSizeInput = splatSize * (2.0f * tan(fovAngleY / 2.0f)) * distanceInputToCamera;
		//	float projectedSplatSizeOutput = splatSize * (2.0f * tan(fovAngleY / 2.0f)) * distanceOutputToCamera;

		//	// Ignore pixels that "shine through" closer surfaces
		//	if (distance(inputPositionScreen.xy, outputPositionScreen.xy) < projectedSplatSizeInput)
		//	{
		//		outputColor = inputColor;
		//		outputImportance = inputImportance;
		//	}
		//}

		//if (pullPushLevel == 1)
		//{
		//	outputImportance = pow(outputImportance, importanceExponent);
		//	outputImportance = clamp(outputImportance, 0, 1);

		//	// Debug importance map
		//	if (drawImportance)
		//	{
		//		outputColor = float4(outputImportance, outputImportance, outputImportance, 1.0f);
		//	}
		//	else
		//	{
		//		outputColor.rgb *= outputImportance;
		//	}
		//}
	}

	outputColorTexture[pixel] = outputColor;
	outputImportanceTexture[pixel] = outputImportance;
}