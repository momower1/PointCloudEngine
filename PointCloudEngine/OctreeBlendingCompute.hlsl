RWTexture2D<float4> renderTargetTexture : register(u0);
Texture2D<uint2> stencilTexture : register(t0);

[numthreads(1, 1, 1)]
void CS (uint3 id : SV_DispatchThreadID)
{
	uint stencil = stencilTexture[id.xy].g;

	if (stencil > 0)
	{
		float4 color = renderTargetTexture[id.xy];

		color -= float4(0.5f, 0.5f, 0.5f, 0);
		color /= stencil;
		color.a = 1;

		renderTargetTexture[id.xy] = color;
	}

	renderTargetTexture[id.xy] = float4(stencil / 255.0f, 0, 0, 1);
}