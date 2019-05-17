#include "ImageEffect.hlsl"

Texture2D<uint2> stencilTexture : register(t0);

float4 PS(GS_INOUTPUT input) : SV_TARGET
{
	uint stencil = stencilTexture[input.position.xy].g;

	if (stencil > 0)
	{
		// Get the pixel color from the render target
		// RGB stores the weighted sum of the blended colors
		// A stores the sum of weights
		float4 color = backBufferTexture[input.position.xy];

		// Remove the background color and alpha
		color -= float4(0.5f, 0.5f, 0.5f, 1.0f);

		// Divide by the sum of weights
		color /= color.a;

		// Make sure that the pixel is opaque
		color.a = 1;

		// Overwrite the old color
		backBufferTexture[input.position.xy] = color;
	}

	return float4(0, 0, 0, 0);
}