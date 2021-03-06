#include "GroundTruth.hlsl"
#include "SplatBlending.hlsl"

[maxvertexcount(6)]
void GS(point VS_OUTPUT input[1], inout TriangleStream<GS_SPLAT_OUTPUT> output)
{
	// Discard splats that face away from the camera
	if (backfaceCulling)
	{
		float3 viewDirection = normalize(input[0].position - cameraPosition);
		float angle = acos(dot(input[0].normal, -viewDirection));

		if (angle > (PI / 2))
		{
			return;
		}
	}

    float3 cameraRight = float3(View[0][0], View[1][0], View[2][0]);
    float3 cameraUp = float3(View[0][1], View[1][1], View[2][1]);
    float3 cameraForward = float3(View[0][2], View[1][2], View[2][2]);

    // Billboard should face in the same direction as the normal
	float splatSizeWorld = length(mul(float3(samplingRate, 0, 0), World).xyz);
    float3 up = 0.5f * splatSizeWorld * normalize(cross(input[0].normal, cameraRight));
    float3 right = 0.5f * splatSizeWorld * normalize(cross(input[0].normal, up));

    float4x4 VP = mul(View, Projection);

	SplatBlendingGS(input[0].position, input[0].normal, input[0].color, up, right, VP, output);
}

float4 PS(GS_SPLAT_OUTPUT input) : SV_TARGET
{
	if (drawNormals)
	{
		// Output and possibly blend the world normal vectors in RGB without lighting
		input.color = 0.5f * ((normalsInScreenSpace ? input.normalScreen : input.normal) + 1);

		return SplatBlendingPS(false, useBlending, cameraPosition, blendFactor, WorldViewProjectionInverse, input);
	}

	return SplatBlendingPS(useLighting, useBlending, cameraPosition, blendFactor, WorldViewProjectionInverse, input);
}