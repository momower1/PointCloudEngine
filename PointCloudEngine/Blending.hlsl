#include "ImageEffect.hlsl"

float4 PS(GS_INOUTPUT input) : SV_TARGET
{
	// Get the pixel color from the render target
	// RGB stores the weighted sum of the blended colors
	// A stores the sum of weights
	float4 color = backBufferTexture[input.position.xy];

	if (color.a > 0)
	{
		// Remove the background color and alpha
		color -= float4(0.5f, 0.5f, 0.5f, 0);

		// Divide by the sum of weights
		color /= color.a;

		// Make sure that the pixel is opaque
		color.a = 1;

		// Overwrite the old color
		backBufferTexture[input.position.xy] = color;
	}

	return float4(0, 0, 0, 0);
}