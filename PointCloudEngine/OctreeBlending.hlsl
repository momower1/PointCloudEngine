#include "ImageEffect.hlsl"

Texture2D<uint2> stencilTexture : register(t0);

float4 PS(GS_INOUTPUT input) : SV_TARGET
{
	uint stencil = stencilTexture[input.position.xy].g;

	if (stencil > 0)
	{
		float4 color = backBufferTexture[input.position.xy];

		color -= float4(0.5f, 0.5f, 0.5f, 1.0f);
		color /= color.a;
		color.a = 1;

		backBufferTexture[input.position.xy] = color;
	}

	return float4(0, 0, 0, 0);
}