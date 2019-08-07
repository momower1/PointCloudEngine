#include "ImageEffect.hlsl"

Texture2D inputTexture : register(t0);

float4 PS(GS_INOUTPUT input) : SV_TARGET
{
	return inputTexture[input.position.xy];
}