RWTexture2D<float4> renderTargetTexture : register(u0);

[numthreads(1, 1, 1)]
void CS (uint3 id : SV_DispatchThreadID)
{
	float4 color = renderTargetTexture[id.xy];

	// Convert from linear space to gamma space
	color = pow(color, 2.2f);

	renderTargetTexture[id.xy] = color;
}