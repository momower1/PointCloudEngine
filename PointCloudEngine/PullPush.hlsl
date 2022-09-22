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
			float4 inputColorTopLeft = inputTexture[2 * pixel];
			float4 inputColorTopRight = inputTexture[2 * pixel + uint2(1, 0)];
			float4 inputColorBottomLeft = inputTexture[2 * pixel + uint2(0, 1)];
			float4 inputColorBottomRight = inputTexture[2 * pixel + uint2(1, 1)];

			if (inputColorTopLeft.w > 0)
			{
				outputColor += inputColorTopLeft;
			}

			if (inputColorTopRight.w > 0)
			{
				outputColor += inputColorTopRight;
			}

			if (inputColorBottomLeft.w > 0)
			{
				outputColor += inputColorBottomLeft;
			}

			if (inputColorBottomRight.w > 0)
			{
				outputColor += inputColorBottomRight;
			}

			if (outputColor.w > 0)
			{
				outputColor /= outputColor.w;
			}
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
			float4 inputColor = inputTexture[pixel / 2];
			outputColor = outputTexture[pixel];
			outputColor = (1.0f - outputColor.w) * inputColor + (outputColor.w * outputColor);
		}
	}

	outputTexture[pixel] = outputColor;
}