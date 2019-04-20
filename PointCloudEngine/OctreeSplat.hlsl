#include "OctreeShaderFunctions.hlsl"

VS_INPUT VS(VS_INPUT input)
{
    return VertexShaderFunction(input);
}

[maxvertexcount(16)]
void GS(point VS_INPUT input[1], inout TriangleStream<GS_OUTPUT> output)
{
	float3 normals[6] =
	{
		PolarNormalToFloat3(input[0].normal0),
		PolarNormalToFloat3(input[0].normal1),
		PolarNormalToFloat3(input[0].normal2),
		PolarNormalToFloat3(input[0].normal3),
		PolarNormalToFloat3(input[0].normal4),
		PolarNormalToFloat3(input[0].normal5)
	};

	float3 colors[6] =
	{
		Color16ToFloat3(input[0].color0),
		Color16ToFloat3(input[0].color1),
		Color16ToFloat3(input[0].color2),
		Color16ToFloat3(input[0].color3),
		Color16ToFloat3(input[0].color4),
		Color16ToFloat3(input[0].color5)
	};

	float weights[6] =
	{
		(input[0].weights & 31) / 31.0f,
		((input[0].weights >> 5) & 31) / 31.0f,
		((input[0].weights >> 10) & 31) / 31.0f,
		((input[0].weights >> 15) & 31) / 31.0f,
		((input[0].weights >> 20) & 31) / 31.0f,
		((input[0].weights >> 25) & 31) / 31.0f
	};

	// Use world inverse to transform camera position into local space
	float4x4 worldInverse = transpose(WorldInverseTranspose);

	// View direction in local space
	float3 viewDirection = normalize(input[0].position - mul(float4(cameraPosition, 1), worldInverse));
	float3 normal = float3(0, 0, 0);
	float3 color = float3(0, 0, 0);

	float visibilityFactorSum = 0;

	// Compute the normal and color from this view direction
	for (int i = 0; i < 6; i++)
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
    float distanceToCamera = distance(cameraPosition, worldPosition);
    float splatSizeWorld = overlapFactor * splatSize * (2.0f * tan(fovAngleY / 2.0f)) * distanceToCamera;
    float3 up = 0.5f * splatSizeWorld * normalize(cross(worldNormal, cameraRight));
    float3 right = 0.5f * splatSizeWorld * normalize(cross(worldNormal, up));

    float4x4 VP = mul(View, Projection);

    GS_OUTPUT element;
    element.color = color;
    element.normal = normal;

	// Append the vertices in the correct order to create a billboard
    element.position = mul(float4(worldPosition + up - right, 1), VP);
    output.Append(element);

    element.position = mul(float4(worldPosition - up + right, 1), VP);
    output.Append(element);

    element.position = mul(float4(worldPosition - up - right, 1), VP);
    output.Append(element);

    output.RestartStrip();

    element.position = mul(float4(worldPosition + up - right, 1), VP);
    output.Append(element);

    element.position = mul(float4(worldPosition + up + right, 1), VP);
    output.Append(element);

    element.position = mul(float4(worldPosition - up + right, 1), VP);
    output.Append(element);
}

float4 PS(GS_OUTPUT input) : SV_TARGET
{
    return PixelShaderFunction(input);
}