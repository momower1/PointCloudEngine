#include "ImageEffect.hlsl"

float4 PS(GS_INOUTPUT input) : SV_TARGET
{
	float4 color = backBufferTexture[input.position.xy];

	// Convert from linear space to gamma space
	color = pow(color, 2.2f);

	backBufferTexture[input.position.xy] = color;

	return float4(0, 0, 0, 0);
}