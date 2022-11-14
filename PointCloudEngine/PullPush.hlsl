#include "GroundTruth.hlsl"
#include "Geometry.hlsli"

//#define DEBUG_SPLAT_TEXEL_OVERLAP
//#define DEBUG_SINGLE_QUAD

SamplerState samplerState : register(s0);
Texture2D<float> depthTexture : register(t0);
Texture2D<float4> inputColorTexture : register(t1);
Texture2D<float4> inputNormalTexture : register(t2);
Texture2D<float4> inputPositionTexture : register(t3);
RWTexture2D<float4> outputColorTexture : register(u0);
RWTexture2D<float4> outputNormalTexture : register(u1);
RWTexture2D<float4> outputPositionTexture : register(u2);

cbuffer PullPushConstantBuffer : register(b1)
{
	bool debug;
	bool isPullPhase;
	bool orientedSplat;
	bool texelBlending;
//------------------------------------------------------------------------------ (16 byte boundary)
	int resolutionPullPush;
	int resolutionOutput;
	int resolutionX;
	int resolutionY;
//------------------------------------------------------------------------------ (16 byte boundary)
	float blendRange;
	float splatSize;
	float nearZ;
	float farZ;
//------------------------------------------------------------------------------ (16 byte boundary)
	int pullPushLevel;
	// 12 bytes auto paddding
//------------------------------------------------------------------------------ (16 byte boundary)
};  // Total: 64 bytes with constant buffer packing rules

void GetTexelCoordinates(in uint2 texelId, inout float2 texelNDC[4])
{
	// Calculate normalized device coordinates for the four texel corners in clockwise order
	uint2 offsets[] =
	{
		uint2(0, 0),	// Top left
		uint2(1, 0),	// Top right
		uint2(1, 1),	// Bottom right
		uint2(0, 1)		// Bottom left
	};

	// Go from UV space to NDC space and invert y coordinate
	for (int i = 0; i < 4; i++)
	{
		texelNDC[i] = resolutionPullPush * ((texelId + offsets[i]) / (float)resolutionOutput);
		texelNDC[i] = 2 * (texelNDC[i] / float2(resolutionX, resolutionY)) - 1;
		texelNDC[i].y = -texelNDC[i].y;
	}
}

void GetProjectedQuad(in float4 quadPositionNDC, in float3 quadNormalWorld, inout float2 quadProjected[4])
{
	float3 cameraRight = float3(View[0][0], View[1][0], View[2][0]);
	float3 cameraUp = float3(View[0][1], View[1][1], View[2][1]);
	float3 cameraForward = float3(View[0][2], View[1][2], View[2][2]);

	// Transform quad position into world space
	float4 quadPositionLocal = mul(float4(quadPositionNDC.xyz, 1), WorldViewProjectionInverse);
	quadPositionLocal /= quadPositionLocal.w;

	float3 quadPositionWorld = mul(quadPositionLocal, World).xyz;
	float3 quadPositionView = mul(float4(quadPositionWorld, 1), View).xyz;

	float3 quadUp;
	float3 quadRight;

	// Construct quad that faces in the same direction as the normal or it should be aligned with the camera
	if (orientedSplat)
	{
		quadUp = 0.5f * splatSize * normalize(cross(quadNormalWorld, cameraRight));
		quadRight = 0.5f * splatSize * normalize(cross(quadNormalWorld, quadUp));
	}
	else
	{
		quadUp = 0.5f * splatSize * cameraUp;
		quadRight = 0.5f * splatSize * cameraRight;
	}

	// Construct quad in clockwise order
	float4 quadNDC[] =
	{
		float4(quadPositionWorld + quadUp - quadRight, 1),	// Top left
		float4(quadPositionWorld + quadUp + quadRight, 1),	// Top right
		float4(quadPositionWorld - quadUp + quadRight, 1),	// Bottom right
		float4(quadPositionWorld - quadUp - quadRight, 1)	// Bottom left
	};

	// Project quad into NDC space
	float4x4 VP = mul(View, Projection);

	for (int i = 0; i < 4; i++)
	{
		quadNDC[i] = mul(quadNDC[i], VP);

		// Homogeneous division
		quadNDC[i] /= quadNDC[i].w;

		// Assign projected 2D quad
		quadProjected[i] = quadNDC[i].xy;
	}
}

[numthreads(32, 32, 1)]
void CS(uint3 id : SV_DispatchThreadID)
{
	if (debug)
	{
		outputColorTexture[id.xy] = inputColorTexture.SampleLevel(samplerState, (id.xy + float2(0.5f, 0.5f)) / resolutionOutput, 0);
		return;
	}

#ifdef DEBUG_SPLAT_TEXEL_OVERLAP
	float2 pixelNDC = resolutionPullPush * ((id.xy + float2(0.5f, 0.5f)) / (float)resolutionOutput);
	pixelNDC = 2 * (pixelNDC / float2(resolutionX, resolutionY)) - 1;
	pixelNDC.y *= -1;

	// Compute overlapping area between a projected quad shaped splat and a texel in screen space (2D)
	float3 quadPositionWorld = float3(0, 0, 1);
	float3 quadNormalWorld = normalize(float3(0, 1, -1));
	float4 quadPositionNDC = mul(mul(float4(quadPositionWorld, 1), View), Projection);
	quadPositionNDC /= quadPositionNDC.w;

	// Project the quad
	float2 quadProjected[4];
	GetProjectedQuad(quadPositionNDC, quadNormalWorld, quadProjected);

	// Construct the texel corners in clockwise order
	float aspectRatio = resolutionX / (float)resolutionY;

	float2 texelNDC[] =
	{
		float2(-0.5f, -0.5f * aspectRatio),		// Top left
		float2(0.5f, -0.5f * aspectRatio),		// Top right
		float2(0.5f, 0.5f * aspectRatio),		// Bottom right
		float2(-0.5f, 0.5f * aspectRatio)		// Bottom left
	};

	float vertexSizeNDC = 0.01f;
	float4 color = { 0, 0, 0, 1 };

	// Color the quad vertices blue
	for (int i = 0; i < 4; i++)
	{
		if (distance(pixelNDC, quadProjected[i]) < vertexSizeNDC)
		{
			color.b += 1.0f;
		}
	}

	// Color the quad green
	if (IsInsideQuad(pixelNDC, quadProjected[0], quadProjected[1], quadProjected[2], quadProjected[3]))
	{
		color.g += 1.0f;
	}

	// Color the texel red
	if (IsInsideQuad(pixelNDC, texelNDC[0], texelNDC[1], texelNDC[2], texelNDC[3]))
	{
		color.r += 1.0f;
	}

	float overlapPercentage = GetTexelQuadOverlapPercentage(texelNDC, quadProjected);

	// The overlap area polygon of two quads can at most have 8 vertices
	float2 overlapVertices[8];
	uint overlapVertexCount = 0;

	GetQuadQuadOverlappingPolygon(texelNDC, quadProjected, overlapVertexCount, overlapVertices);

	if (overlapVertexCount >= 3)
	{
		SortConvexPolygonVerticesClockwise(overlapVertexCount, overlapVertices);

		// Visualize result of fan triangulation
		for (int i = 0; i < overlapVertexCount - 2; i++)
		{
			if (IsInsideTriangle(pixelNDC, overlapVertices[0], overlapVertices[i + 1], overlapVertices[i + 2]))
			{
				color.rgb = float3((i + 1) / (float)(overlapVertexCount - 2), (i + 1) / (float)(overlapVertexCount - 2), 0);
			}
		}
	}

	// Visualize covered area percentage with background color
	if (color.r <= 0.0f && color.g <= 0.0f && color.b <= 0.0f)
	{
		color = float4(overlapPercentage, overlapPercentage, overlapPercentage, 1);
	}

	outputColorTexture[id.xy] = color;
	return;
#endif

#ifdef DEBUG_SINGLE_QUAD
	float3 quadPositionWorld = float3(0, -1, 1);
	float3 quadNormalWorld = float3(0, 1, 0);

	float4 quadPositionNDC = mul(mul(float4(quadPositionWorld, 1), View), Projection);
	quadPositionNDC /= quadPositionNDC.w;

	if (quadPositionNDC.z < 0)
	{
		outputColorTexture[id.xy] = float4(1, 0, 0, 1);
		return;
	}

	float3 viewDirection = normalize(quadPositionWorld - cameraPosition);
	float angle = acos(dot(quadNormalWorld, -viewDirection));

	if (angle > (PI / 2))
	{
		outputColorTexture[id.xy] = float4(0.5f, 0, 1, 1);
		return;
	}

	// Project quad
	float2 quadProjected[4];
	GetProjectedQuad(quadPositionNDC, quadNormalWorld, quadProjected);

	// Construct texel to test against
	float2 texelNDC[4];
	GetTexelCoordinates(id.xy, texelNDC);

	// Only color the texel if all of its texel corners are inside the projected quad
	if (IsInsideQuad(texelNDC[0], quadProjected[0], quadProjected[1], quadProjected[2], quadProjected[3])
		&& IsInsideQuad(texelNDC[1], quadProjected[0], quadProjected[1], quadProjected[2], quadProjected[3])
		&& IsInsideQuad(texelNDC[2], quadProjected[0], quadProjected[1], quadProjected[2], quadProjected[3])
		&& IsInsideQuad(texelNDC[3], quadProjected[0], quadProjected[1], quadProjected[2], quadProjected[3]))
	{
		outputColorTexture[id.xy] = float4(0, 1, 0, 1);
	}
	else
	{
		outputColorTexture[id.xy] = float4(0, 0, 0, 1);
	}

	return;
#endif

	uint2 pixel = id.xy;
	float4 outputColor;
	float4 outputNormal;
	float4 outputPosition;

	if (isPullPhase)
	{
		// Initialize the textures, if there was no point rendered to a pixel, its last texture element component must be zero
		if (pullPushLevel == 0)
		{
			outputColor = float4(0, 0, 0, 0);
			outputNormal = float4(0, 0, 0, 0);
			outputPosition = float4(0, 0, 0, 0);

			if ((pixel.x < resolutionX) && (pixel.y < resolutionY))
			{
				float depth = depthTexture[pixel];

				if (depth < 1.0f)
				{
					// Color of the point
					outputColor = outputColorTexture[pixel];
					outputColor.w = 1.0f;

					// World space normals of the point
					outputNormal = outputNormalTexture[pixel];
					outputNormal.xyz = (2 * outputNormal.xyz) - 1;
					outputNormal.w = 1.0f;

					// Normalized device coordinates of the point
					outputPosition.x = 2 * ((pixel.x + 0.5f) / resolutionX) - 1;
					outputPosition.y = 2 * ((pixel.y + 0.5f) / resolutionY) - 1;
					outputPosition.y = -outputPosition.y;
					outputPosition.z = depth;
					outputPosition.w = 1.0f;
				}
			}
		}
		else
		{
			outputColor = float4(0, 0, 0, 0);
			outputNormal = float4(0, 0, 0, 0);
			outputPosition = float4(0, 0, 0, 0);

			float2 texelNDC[4];
			GetTexelCoordinates(id.xy, texelNDC);

			float surfaceZ = farZ;
			float largestOverlapPercentage = 0;

			// Go over the 2x2 input texels
			for (int y = 0; y < 2; y++)
			{
				for (int x = 0; x < 2; x++)
				{
					float4 inputColor = inputColorTexture[2 * pixel + uint2(x, y)];
					float4 inputNormal = inputNormalTexture[2 * pixel + uint2(x, y)];
					float4 inputPosition = inputPositionTexture[2 * pixel + uint2(x, y)];

					// Only perform calculation if the input is non-empty
					if (inputPosition.w > 0.0f)
					{
						// Project the quad that corresponds to this point
						float2 quadProjected[4];
						GetProjectedQuad(inputPosition, inputNormal.xyz, quadProjected);

						// Get the overlapping area percentage between the quad and the texel
						float overlapPercentage = GetTexelQuadOverlapPercentage(texelNDC, quadProjected);

						// Splat covers the yet largest area of the texel so assign it as the new surface splat
						if (overlapPercentage > largestOverlapPercentage)
						{
							largestOverlapPercentage = overlapPercentage;
							outputNormal = inputNormal;
							outputPosition = inputPosition;

							if (!texelBlending)
							{
								outputColor = inputColor;
							}
						}

						// Blend splat colors together
						if (texelBlending)
						{
							// Accumulate weighted colors and weights
							float blendWeight = overlapPercentage;
							outputColor.rgb += blendWeight * inputColor.rgb;
							outputColor.w += blendWeight * inputColor.w;
						}
					}
				}
			}

			// Normalize accumulated blended color
			if (texelBlending && (outputColor.w > 0.0f))
			{
				outputColor /= outputColor.w;
			}
		}
	}
	else
	{
		float2 uv = (pixel + float2(0.5f, 0.5f)) / resolutionOutput;
		float4 inputColor = inputColorTexture.SampleLevel(samplerState, uv, 0);
		float4 inputNormal = inputNormalTexture[pixel / 2];
		float4 inputPosition = inputPositionTexture[pixel / 2];
		
		outputColor = outputColorTexture[pixel];
		outputNormal = outputNormalTexture[pixel];
		outputPosition = outputPositionTexture[pixel];

		// Replace pixels where no point has been rendered to (therefore 4th component is zero) or pixels that are obscured by a closer surface from the higher level pull texture
		if (inputPosition.w >= 1.0f)
		{
			float2 texelNDC[4];
			GetTexelCoordinates(id.xy, texelNDC);

			// Project the quad that corresponds to the input point
			float2 inputQuadProjected[4];
			GetProjectedQuad(inputPosition, inputNormal.xyz, inputQuadProjected);

			float inputOverlapPercentage = GetTexelQuadOverlapPercentage(texelNDC, inputQuadProjected);

			// Need to compare overlay percentages of input and output, only keep splat with larger overlay
			if (outputPosition.w >= 0)
			{
				// Project the quad that corresponds to the output point
				float2 outputQuadProjected[4];
				GetProjectedQuad(outputPosition, outputNormal.xyz, outputQuadProjected);

				float outputOverlapPercentage = GetTexelQuadOverlapPercentage(texelNDC, outputQuadProjected);

				if (inputOverlapPercentage > outputOverlapPercentage)
				{
					outputNormal = inputNormal;
					outputPosition = inputPosition;

					if (!texelBlending)
					{
						outputColor = inputColor;
					}
				}

				if (texelBlending)
				{
					float blendWeight = inputOverlapPercentage;
					outputColor.rgb += blendWeight * inputColor.rgb;
					outputColor.w += blendWeight * inputColor.w;
				}
				else if (inputOverlapPercentage >= 0.99f)
				{
					outputColor = inputColor;
				}
			}

			// Normalize accumulated blended color
			if (texelBlending && (outputColor.w > 0.0f))
			{
				outputColor /= outputColor.w;
			}
		}
	}

	outputColorTexture[pixel] = outputColor;
	outputNormalTexture[pixel] = outputNormal;
	outputPositionTexture[pixel] = outputPosition;
}