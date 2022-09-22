Texture2D<float> depthTexture : register(t0);
RWTexture2D<float4> inputTexture : register(u1);
RWTexture2D<float4> outputTexture : register(u2);

cbuffer PullPushConstantBuffer : register(b0)
{
	int pullPushLevel;
	bool isPullPhase;
	// 8 bytes auto padding
//------------------------------------------------------------------------------ (16 byte boundary)
};  // Total: 16 bytes with constant buffer packing rules

[numthreads(32, 32, 1)]
void CS(uint3 id : SV_DispatchThreadID)
{
}