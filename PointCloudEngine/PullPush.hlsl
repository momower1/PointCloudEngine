Texture2D<float> depthTexture : register(t0);
Texture2D<float4> inputColorTexture : register(t1);
Texture2D<float> inputImportanceTexture : register(t2);
RWTexture2D<float4> outputColorTexture : register(u0);
RWTexture2D<float> outputImportanceTexture : register(u1);
SamplerState samplerState : register(s0);

cbuffer PullPushConstantBuffer : register(b0)
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
};  // Total: 16 bytes with constant buffer packing rules

[numthreads(32, 32, 1)]
void CS(uint3 id : SV_DispatchThreadID)
{
	uint2 pixel = id.xy;
	float4 outputColor;
	float outputImportance;

	if (isPullPhase)
	{
		// Initialize the texture such that each pixel contains the point color in the RGB and its depth in the A component
		// If there was no point rendered to a pixel, its depth must be 1
		if (pullPushLevel == 0)
		{
			if ((pixel.x < resolutionX) && (pixel.y < resolutionY))
			{
				outputColor = outputColorTexture[pixel];
				outputColor.w = depthTexture[pixel];
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

					if (inputColor.w <= smallestDepth)
					{
						smallestDepth = inputColor.w;
						outputColor = inputColor;
					}
				}
			}
		}

		outputImportance = (1.0f + importanceScale) / (1 + pullPushLevel);
	}
	else
	{
		float2 uv = (pixel + float2(0.5f, 0.5f)) / resolutionOutput;
		float4 inputColor = inputColorTexture.SampleLevel(samplerState, uv, 0);
		float inputImportance = inputImportanceTexture.SampleLevel(samplerState, uv, 0);
		
		outputColor = outputColorTexture[pixel];
		outputImportance = outputImportanceTexture[pixel] + inputImportance;

		// Only replace pixels where no point has been rendered to (therefore depth is 1) with values from the higher level pull texture
		if (outputColor.w >= 1.0f)
		{
			outputColor = inputColor;
			outputImportance = inputImportance;
		}
		// Ignore pixels that shine through the closest surface and replace them
		else if ((outputColor.w - inputColor.w) > depthBias)
		{
			outputColor = inputColor;
		}

		if (pullPushLevel == 1)
		{
			outputImportance = pow(outputImportance, importanceExponent);
			outputImportance = clamp(outputImportance, 0, 1);

			// Debug importance map
			if (drawImportance)
			{
				outputColor = float4(outputImportance, outputImportance, outputImportance, 1.0f);
			}
			else
			{
				outputColor.rgb *= outputImportance;
			}
		}
	}

	outputColorTexture[pixel] = outputColor;
	outputImportanceTexture[pixel] = outputImportance;
}