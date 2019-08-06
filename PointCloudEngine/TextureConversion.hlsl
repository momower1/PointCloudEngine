#include "ImageEffect.hlsl"

Texture2D inputTexture : register(t0);

float4 PS(GS_INOUTPUT input) : SV_TARGET
{
	// Convert back to linear space from gamma space
	return pow(inputTexture[input.position.xy], 1.0f / 2.2f);
}