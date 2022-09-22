Texture2D<float> depthTexture : register(t0);
RWTexture2D<float4> inputTexture : register(u0);
RWTexture2D<float4> outputTexture : register(u1);

cbuffer PullPushConstantBuffer : register(b0)
{
	int resolutionX;
	int resolutionY;
	int pullPushLevel;
	bool isPullPhase;
//------------------------------------------------------------------------------ (16 byte boundary)
};  // Total: 16 bytes with constant buffer packing rules

[numthreads(32, 32, 1)]
void CS(uint3 id : SV_DispatchThreadID)
{
	uint2 pixel = id.xy;
	float4 outputColor = float4(0, 0, 0, 0);

	if (isPullPhase)
	{
		// Copy input texture to output texture and set valid weight if there is a depth value
		if (pullPushLevel == 0)
		{
			if ((pixel.x < resolutionX) && (pixel.y < resolutionY) && (depthTexture[pixel] < 1.0f))
			{
				outputColor = inputTexture[pixel];
				outputColor.w = 1.0f;
			}
		}
		else
		{
			outputColor = 0.25f * inputTexture[2 * pixel];
			outputColor += 0.25f * inputTexture[2 * pixel + uint2(1, 0)];
			outputColor += 0.25f * inputTexture[2 * pixel + uint2(0, 1)];
			outputColor += 0.25f * inputTexture[2 * pixel + uint2(1, 1)];
		}
	}
	else
	{
		// Copy input texture to output texture
		if (pullPushLevel == 0)
		{
			outputColor = inputTexture[pixel];
		}
		else
		{
			outputColor = inputTexture[pixel / 2];
		}
	}

	outputTexture[pixel] = outputColor;
}