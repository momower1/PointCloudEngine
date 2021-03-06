#include "Octree.hlsl"
#include "OctreeConstantBuffer.hlsl"
#include "SplatBlending.hlsl"

VS_INPUT VS(VS_INPUT input)
{
    return input;
}

[maxvertexcount(16)]
void GS(point VS_INPUT input[1], inout TriangleStream<GS_SPLAT_OUTPUT> output)
{
	float3 normals[4] =
	{
		ClusterNormalToFloat4(input[0].normal0).xyz,
		ClusterNormalToFloat4(input[0].normal1).xyz,
		ClusterNormalToFloat4(input[0].normal2).xyz,
		ClusterNormalToFloat4(input[0].normal3).xyz
	};

	float3 colors[4] =
	{
		Color16ToFloat3(input[0].color0),
		Color16ToFloat3(input[0].color1),
		Color16ToFloat3(input[0].color2),
		Color16ToFloat3(input[0].color3)
	};

	float weights[4] =
	{
		input[0].weight0 / 255.0f,
		input[0].weight1 / 255.0f,
		input[0].weight2 / 255.0f,
		1.0f - (input[0].weight0 + input[0].weight1 + input[0].weight2) / 255.0f
	};

	// Use world inverse to transform camera position into local space
	float4x4 worldInverse = transpose(WorldInverseTranspose);

	// View direction in local space
	float3 viewDirection = normalize(input[0].position - mul(float4(cameraPosition, 1), worldInverse));
	float3 normal = float3(0, 0, 0);
	float3 color = float3(0, 0, 0);

	float visibilityFactorSum = 0;

	// Compute the normal and color from this view direction
	for (int i = 0; i < 4; i++)
	{
		float visibilityFactor = weights[i] * dot(normals[i], -viewDirection);

		if (visibilityFactor > 0)
		{
			normal += visibilityFactor * normals[i];
			color += visibilityFactor * colors[i];

			visibilityFactorSum += visibilityFactor;
		}
	}

	if (visibilityFactorSum > 0)
	{
		normal /= visibilityFactorSum;
		color /= visibilityFactorSum;

		float3 worldPosition = mul(float4(input[0].position, 1), World);
		float3 worldNormal = normalize(mul(float4(normal, 0), WorldInverseTranspose));

		float3 cameraRight = float3(View[0][0], View[1][0], View[2][0]);
		float3 cameraUp = float3(View[0][1], View[1][1], View[2][1]);
		float3 cameraForward = float3(View[0][2], View[1][2], View[2][2]);

		// Billboard should face in the same direction as the normal
		// Also the size should not go below the sampling rate in order to avoid holes
		float distanceToCamera = distance(cameraPosition, worldPosition);
		float samplingRateWorld = length(mul(float3(samplingRate, 0, 0), World).xyz);
		float splatResolutionWorld = overlapFactor * splatResolution * (2.0f * tan(fovAngleY / 2.0f)) * distanceToCamera;
		float splatSizeWorld = max(samplingRateWorld, splatResolutionWorld);
		float3 up = 0.5f * splatSizeWorld * normalize(cross(worldNormal, cameraRight));
		float3 right = 0.5f * splatSizeWorld * normalize(cross(worldNormal, up));
		float4x4 VP = mul(View, Projection);

		SplatBlendingGS(worldPosition, worldNormal, color, up, right, VP, output);
	}
}

float4 PS(GS_SPLAT_OUTPUT input) : SV_TARGET
{
	return SplatBlendingPS(useLighting, useBlending, cameraPosition, blendFactor, WorldViewProjectionInverse, input);
}