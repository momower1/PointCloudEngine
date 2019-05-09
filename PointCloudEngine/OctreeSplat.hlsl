#include "Octree.hlsl"
#include "OctreeConstantBuffer.hlsl"
#include "LightingConstantBuffer.hlsl"

Texture2D<float> octreeDepthTexture : register(t2);

SamplerState PointSampler
{
	Filter = MIN_MAG_MIP_POINT;
	AddressU = Clamp;
	AddressV = Clamp;
};

struct GS_SPLAT_OUTPUT
{
	float4 position : SV_POSITION;
	float4 positionClip : POSITION1;
	float3 positionWorld : POSITION2;
	float3 positionCenter : POSITION3;
	float3 normal : NORMAL;
	float3 color : COLOR;
	float radius : RADIUS;
};

VS_INPUT VS(VS_INPUT input)
{
    return input;
}

[maxvertexcount(16)]
void GS(point VS_INPUT input[1], inout TriangleStream<GS_SPLAT_OUTPUT> output)
{
	float3 normals[4] =
	{
		PolarNormalToFloat3(input[0].normal0),
		PolarNormalToFloat3(input[0].normal1),
		PolarNormalToFloat3(input[0].normal2),
		PolarNormalToFloat3(input[0].normal3)
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

	normal /= visibilityFactorSum;
	color /= visibilityFactorSum;

    /*

        1,4__5
        |\   |
        | \  |
        |  \ |
        |___\|
        3    2,6

    */

    float3 worldPosition = mul(float4(input[0].position, 1), World);
    float3 worldNormal = normalize(mul(float4(normal, 0), WorldInverseTranspose));

    float3 cameraRight = float3(View[0][0], View[1][0], View[2][0]);
    float3 cameraUp = float3(View[0][1], View[1][1], View[2][1]);
    float3 cameraForward = float3(View[0][2], View[1][2], View[2][2]);

    // Billboard should face in the same direction as the normal
	// Also the size should not go below the sampling rate in order to avoid holes
    float distanceToCamera = distance(cameraPosition, worldPosition);
    float sizeWorld = length(mul(float3(samplingRate, 0, 0), World).xyz);
	float splatSizeWorld = overlapFactor * splatSize * (2.0f * tan(fovAngleY / 2.0f)) * distanceToCamera;
	float billboardSize = max(sizeWorld, splatSizeWorld);
    float3 up = 0.5f * billboardSize * normalize(cross(worldNormal, cameraRight));
    float3 right = 0.5f * billboardSize * normalize(cross(worldNormal, up));

    float4x4 VP = mul(View, Projection);

    GS_SPLAT_OUTPUT element;
	element.positionCenter = worldPosition;
	element.normal = worldNormal;
    element.color = color;
	element.radius = length(up);

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

float4 PS(GS_SPLAT_OUTPUT input) : SV_TARGET
{
	// Make circular splats, remove this for squares
	if (distance(input.positionWorld, input.positionCenter) > input.radius)
	{
		discard;
	}

	if (blend)
	{
		// Transform from clip position into texture space
		float3 positionClip = input.positionClip.xyz / input.positionClip.w;
		float2 uv = float2(positionClip.x / 2.0f, positionClip.y / -2.0f) + 0.5f;

		// Transform back into object space because comparing the depth values directly doesn't work well since they are not distributed linearely
		float4 position = mul(float4(positionClip.xyz, 1), WorldViewProjectionInverse);
		position = position / position.w;

		// This is the depth of the rendered surface to compare against
		float surfaceDepth = octreeDepthTexture.Sample(PointSampler, uv);
		float4 surfacePosition = mul(float4(positionClip.x, positionClip.y, surfaceDepth, 1), WorldViewProjectionInverse);
		surfacePosition = surfacePosition / surfacePosition.w;

		// Discard this pixel if it is not close to the surface
		if (distance(position, surfacePosition) > depthEpsilon)
		{
			discard;
		}
	}

	if (light)
	{
		return PhongLighting(cameraPosition, input.positionWorld, input.normal, input.color.rgb);
	}
	else
	{
		return float4(input.color.rgb, 1);
	}
}