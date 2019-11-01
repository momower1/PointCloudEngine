#include "LightingConstantBuffer.hlsl"

Texture2D<float> blendingDepthTexture : register(t0);

struct GS_SPLAT_OUTPUT
{
	float4 position : SV_POSITION;
	float4 positionClip : POSITION1;
	float3 positionWorld : POSITION2;
	float3 positionCenter : POSITION3;
	float3 normalScreen : NORMAL_SCREEN;
	float3 normal : NORMAL;
	float3 color : COLOR;
	float radius : RADIUS;
};

SamplerState PointSampler
{
	Filter = MIN_MAG_MIP_POINT;
	AddressU = Clamp;
	AddressV = Clamp;
};

float Gaussian(float x, float o)
{
	return (1.0f / (o * sqrt(2 * 3.14159265359))) * pow(2.71828182846, -((x * x) / (2 * o * o)));
}

void SplatBlendingGS(float3 worldPosition, float3 worldNormal, float3 color, float3 up, float3 right, float4x4 VP, bool backfaceCulling, inout TriangleStream<GS_SPLAT_OUTPUT> output)
{
	/*

		1,4__5
		|\   |
		| \  |
		|  \ |
		|___\|
		3    2,6

	*/

	GS_SPLAT_OUTPUT element;
	element.positionCenter = worldPosition;
	element.normalScreen = normalize(mul(worldNormal, VP));
	element.normalScreen.z *= -1;
	element.normal = worldNormal;
	element.color = color;
	element.radius = length(up);

	// Perform backface culling based on the normal
	if (!backfaceCulling || (element.normalScreen.z > 0))
	{
		// Append the vertices in the correct order to create a billboard
		element.positionWorld = worldPosition + up - right;
		element.position = element.positionClip = mul(float4(element.positionWorld, 1), VP);
		output.Append(element);

		element.positionWorld = worldPosition - up + right;
		element.position = element.positionClip = mul(float4(element.positionWorld, 1), VP);
		output.Append(element);

		element.positionWorld = worldPosition - up - right;
		element.position = element.positionClip = mul(float4(element.positionWorld, 1), VP);
		output.Append(element);

		output.RestartStrip();

		element.positionWorld = worldPosition + up - right;
		element.position = element.positionClip = mul(float4(element.positionWorld, 1), VP);
		output.Append(element);

		element.positionWorld = worldPosition + up + right;
		element.position = element.positionClip = mul(float4(element.positionWorld, 1), VP);
		output.Append(element);

		element.positionWorld = worldPosition - up + right;
		element.position = element.positionClip = mul(float4(element.positionWorld, 1), VP);
		output.Append(element);
	}
}

float4 SplatBlendingPS(bool useLighting, bool useBlending, float3 cameraPosition, float blendFactor, float4x4 WVPI, GS_SPLAT_OUTPUT input)
{
	// This is the final pixel color with or without lighting
	float3 color;

	// Use this factor to make circular splats and to calculate the blending weight
	float factor = distance(input.positionWorld, input.positionCenter) / input.radius;

	// Make circular splats, remove this for squares
	if (factor > 1)
	{
		discard;
	}

	// Calculate the final pixel color
	if (useLighting)
	{
		color = PhongLighting(cameraPosition, input.positionWorld, input.normal, input.color.rgb);
	}
	else
	{
		color = input.color.rgb;
	}

	if (useBlending)
	{
		// Transform from clip position into texture space
		float3 positionClip = input.positionClip.xyz / input.positionClip.w;
		float2 uv = float2(positionClip.x / 2.0f, positionClip.y / -2.0f) + 0.5f;

		// Transform back into object space because comparing the depth values directly doesn't work well since they are not distributed linearely
		float4 position = mul(float4(positionClip.xyz, 1), WVPI);
		position = position / position.w;

		// This is the depth of the rendered surface to compare against
		float surfaceDepth = blendingDepthTexture.Sample(PointSampler, uv);
		float4 surfacePosition = mul(float4(positionClip.x, positionClip.y, surfaceDepth, 1), WVPI);
		surfacePosition = surfacePosition / surfacePosition.w;

		// Discard this pixel if it is not close enough to the surface
		// Use the splat radius as cutoff, a constant value does not work for octree nodes because the distances between the splats become larger with the splat radius
		if (distance(position, surfacePosition) > blendFactor * input.radius)
		{
			discard;
		}

		// Blending weight should be 1 in the center and close to 0 at the edge (0 at the edge introduces artifacts)
		// This weight is blended additively into the alpha channel of the render target and therefore stores the sum of all weights
		float weight = Gaussian(1.2f * factor, 0.4f);

		// Blend the weighted color and the weight additively together
		return float4(weight * color, weight);
	}

	return float4(color, 1);
}