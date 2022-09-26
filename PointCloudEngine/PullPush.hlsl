Texture2D<float> depthTexture : register(t0);
Texture2D<float4> inputTexture : register(t1);
RWTexture2D<float4> outputTexture : register(u0);
RWTexture2D<float4> backbufferTexture : register(u1);
SamplerState samplerState : register(s0);

cbuffer PullPushConstantBuffer : register(b0)
{
	int resolutionX;
	int resolutionY;
	int resolutionOutput;
	int pullPushLevel;
//------------------------------------------------------------------------------ (16 byte boundary)
	bool isPullPhase;
	// 12 byte auto padding
//------------------------------------------------------------------------------ (16 byte boundary)
};  // Total: 16 bytes with constant buffer packing rules

[numthreads(32, 32, 1)]
void CS(uint3 id : SV_DispatchThreadID)
{
	uint2 pixel = id.xy;
	float4 outputColor;

	if (isPullPhase)
	{
		// Copy input texture to output texture and set valid weight if there is a depth value
		if (pullPushLevel == 0)
		{
			if ((pixel.x < resolutionX) && (pixel.y < resolutionY))
			{
				outputColor = backbufferTexture[pixel];
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
					float4 inputColor = inputTexture[2 * pixel + uint2(x, y)];

					if (inputColor.w <= smallestDepth)
					{
						smallestDepth = inputColor.w;
						outputColor = inputColor;
					}
				}
			}
		}
	}
	else
	{
		// Copy input texture to output texture
		if (pullPushLevel == 0)
		{
			outputColor = inputTexture[pixel];
			outputColor.a = 1.0f;

			backbufferTexture[pixel] = outputColor;
			return;
		}
		else
		{
			float2 uv = (pixel + float2(0.5f, 0.5f)) / resolutionOutput;
			float4 inputColor = inputTexture.SampleLevel(samplerState, uv, 0);
			outputColor = outputTexture[pixel];

			// Only replace pixels where no point has been rendered to (therefore depth is 1) with values from the higher level pull texture
			if (outputColor.w >= 1.0f)
			{
				outputColor = inputColor;
			}
		}
	}

	outputTexture[pixel] = outputColor;
}