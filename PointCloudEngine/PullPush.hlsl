#include "GroundTruth.hlsl"

#define DEBUG_SPLAT_TEXEL_OVERLAP
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

float2 GetPerpendicularVector(float2 v)
{
	return float2(v.y, -v.x);
}

float GetSignedAngle(float2 u, float2 v)
{
	// Returns either clockwise or counter-clockwise signed angle in [-PI, PI] depending on coordinate system
	return atan2(u.x * v.y - u.y * v.x, u.x * v.x + u.y * v.y);
}

float GetCounterclockwiseAngle(float2 u, float2 v)
{
	float angle = GetSignedAngle(u, v);
	return (angle < 0) ? (2 * PI + angle) : angle;
}

float2 FlipVectorIntoDirection(float2 v, float2 direction)
{
	return v * sign(dot(v, direction));
}

float2 GetLineLineIntersection(float2 o1, float2 d1, float2 o2, float2 d2)
{
	// Assume that the lines are not parallel (otherwise division by zero)
	// Both lines have origin and direction, calculate distance along second line to the intersection point
	float lambda2 = (d1.x * o2.y - d1.x * o1.y - d1.y * o2.x + d1.y * o1.x) / (d1.y * d2.x - d1.x * d2.y);

	return o2 + lambda2 * d2;
}

bool GetLineSegmentIntersection(float2 o1, float2 d1, float2 o2, float2 d2, out float2 intersection)
{
	// Assume that the lines are not parallel (otherwise division by zero)
	// Both lines have origin and direction, calculate distance along second line to the intersection point
	// Need to explicitly calculate both lambdas, otherwise there is a configuration where one of them is incorrect
	float lambda1 = (d2.x * o1.y - d2.x * o2.y - d2.y * o1.x + d2.y * o2.x) / (d2.y * d1.x - d2.x * d1.y);
	float lambda2 = (d1.x * o2.y - d1.x * o1.y - d1.y * o2.x + d1.y * o1.x) / (d1.y * d2.x - d1.x * d2.y);
	intersection = o2 + lambda2 * d2;

	return (lambda1 >= 0) && (lambda1 <= 1) && (lambda2 >= 0) && (lambda2 <= 1);
}

bool IsInsideQuad(float2 position, float2 quadTopLeft, float2 quadBottomRight, float2 quadLeftNormal, float2 quadTopNormal, float2 quadRightNormal, float2 quadBottomNormal)
{
	bool isInsideTopEdge = dot(position - quadTopLeft, quadTopNormal) > 0;
	bool isInsideLeftEdge = dot(position - quadTopLeft, quadLeftNormal) > 0;
	bool isInsideRightEdge = dot(position - quadBottomRight, quadRightNormal) > 0;
	bool isInsideBottomEdge = dot(position - quadBottomRight, quadBottomNormal) > 0;

	return isInsideTopEdge && isInsideLeftEdge && isInsideRightEdge && isInsideBottomEdge;
}

bool IsInsideQuad(float2 position, float2 quadTopLeft, float2 quadTopRight, float2 quadBottomLeft, float2 quadBottomRight)
{
	// Construct quad edges
	float2 eLeft = quadTopLeft - quadBottomLeft;
	float2 eRight = quadTopRight - quadBottomRight;
	float2 eTop = quadTopRight - quadTopLeft;
	float2 eBottom = quadBottomRight - quadBottomLeft;

	// Get normal vector perpendicular to each edge
	float2 nLeft = GetPerpendicularVector(eLeft);
	float2 nRight = GetPerpendicularVector(eRight);
	float2 nTop = GetPerpendicularVector(eTop);
	float2 nBottom = GetPerpendicularVector(eBottom);

	// Make sure that the edge normals point towards the quad center
	float2 center = (quadTopLeft + quadTopRight + quadBottomLeft + quadBottomRight) / 4.0f;
	nLeft = FlipVectorIntoDirection(nLeft, center - quadTopLeft);
	nRight = FlipVectorIntoDirection(nRight, center - quadTopRight);
	nTop = FlipVectorIntoDirection(nTop, center - quadTopRight);
	nBottom = FlipVectorIntoDirection(nBottom, center - quadBottomRight);

	// Check on which side of each quad edge the point lies
	bool isInsideLeftEdge = dot(position - quadTopLeft, nLeft) > 0;
	bool isInsideRightEdge = dot(position - quadBottomRight, nRight) > 0;
	bool isInsideTopEdge = dot(position - quadTopLeft, nTop) > 0;
	bool isInsideBottomEdge = dot(position - quadBottomRight, nBottom) > 0;

	// The point can only be inside the quad if it is inside all edges
	return isInsideLeftEdge && isInsideRightEdge && isInsideTopEdge && isInsideBottomEdge;
}

bool IsInsideTriangle(float2 position, float2 t1, float2 t2, float2 t3)
{
	// Construct triangle edges
	float2 e12 = t2 - t1;
	float2 e13 = t3 - t1;
	float2 e23 = t3 - t2;

	// Get normal vector perpendicular to each edge
	float2 n12 = GetPerpendicularVector(e12);
	float2 n13 = GetPerpendicularVector(e13);
	float2 n23 = GetPerpendicularVector(e23);

	// Make sure that the edge normals point towards the triangle center
	float2 center = (t1 + t2 + t3) / 3.0f;
	n12 = FlipVectorIntoDirection(n12, center - t1);
	n13 = FlipVectorIntoDirection(n13, center - t1);
	n23 = FlipVectorIntoDirection(n23, center - t2);

	// Check on which side of each triangle edge the point lies
	bool isInsideEdge12 = dot(position - t1, n12) > 0;
	bool isInsideEdge13 = dot(position - t1, n13) > 0;
	bool isInsideEdge23 = dot(position - t3, n23) > 0;

	// The point can only be inside the triangle if it is inside all edges
	return isInsideEdge12 && isInsideEdge13 && isInsideEdge23;
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
	// Compute overlapping area between a projected quad shaped splat and a texel in screen space (2D)
	float3 quadPositionWorld = float3(0, 0, 1);
	float3 quadNormalWorld = normalize(float3(0, 1, -1));

	// Construct a quad in world space
	float3 cameraRight = float3(View[0][0], View[1][0], View[2][0]);
	float3 quadUp = 0.5f * splatSize * normalize(cross(quadNormalWorld, cameraRight));
	float3 quadRight = 0.5f * splatSize * normalize(cross(quadNormalWorld, quadUp));;

	float3 quadTopLeft = quadPositionWorld + quadUp - quadRight;
	float3 quadTopRight = quadPositionWorld + quadUp + quadRight;
	float3 quadBottomLeft = quadPositionWorld - quadUp - quadRight;
	float3 quadBottomRight = quadPositionWorld - quadUp + quadRight;

	// Project quad
	float4x4 VP = mul(View, Projection);
	float4 quadTopLeftNDC = mul(float4(quadTopLeft, 1), VP);
	float4 quadTopRightNDC = mul(float4(quadTopRight, 1), VP);
	float4 quadBottomLeftNDC = mul(float4(quadBottomLeft, 1), VP);
	float4 quadBottomRightNDC = mul(float4(quadBottomRight, 1), VP);

	// Homogeneous division
	quadTopLeftNDC /= quadTopLeftNDC.w;
	quadTopRightNDC /= quadTopRightNDC.w;
	quadBottomLeftNDC /= quadBottomLeftNDC.w;
	quadBottomRightNDC /= quadBottomRightNDC.w;

	float2 texelNDC = resolutionPullPush * ((id.xy + float2(0.5f, 0.5f)) / (float)resolutionOutput);
	texelNDC = 2 * (texelNDC / float2(resolutionX, resolutionY)) - 1;
	texelNDC.y *= -1;

	float4 color = { 0, 0, 0, 1 };
	float vertexSizeNDC = 0.01f;

	// Color the quad vertices blue
	if ((distance(texelNDC, quadTopLeftNDC.xy) < vertexSizeNDC)
		|| (distance(texelNDC, quadTopRightNDC.xy) < vertexSizeNDC)
		|| (distance(texelNDC, quadBottomLeftNDC.xy) < vertexSizeNDC)
		|| (distance(texelNDC, quadBottomRightNDC.xy) < vertexSizeNDC))
	{
		color.b += 1.0f;
	}

	// Color the quad green
	if (IsInsideQuad(texelNDC, quadTopLeftNDC.xy, quadTopRightNDC.xy, quadBottomLeftNDC.xy, quadBottomRightNDC.xy))
	{
		color.g += 1.0f;
	}

	float aspectRatio = resolutionX / (float)resolutionY;
	float2 texelTopLeftNDC = { -0.5f, -0.5f * aspectRatio };
	float2 texelTopRightNDC = { 0.5f, -0.5f * aspectRatio };
	float2 texelBottomLeftNDC = { -0.5f, 0.5f * aspectRatio };
	float2 texelBottomRightNDC = { 0.5f, 0.5f * aspectRatio };

	// Color the texel red
	if (IsInsideQuad(texelNDC, texelTopLeftNDC, texelTopRightNDC, texelBottomLeftNDC, texelBottomRightNDC))
	{
		color.r += 1.0f;
	}

	// Idea to make sure that this works correctly, no need to do rasterization :-)
	// - make sure that point inside triangle check works correctly
	// - compute triangle-triangle area intersection and represent the intersection area as a set of triangles
	// - for each of the area triangles, perform point inside triangle check and verify result visually

	// Brute force idea for filling the overlap area with triangles
	// - just take any 3 intersection points (or contained vertex) and create first triangle
	// - take other 3 intersection points and add triangle if it does not intersect with any previously added triangle
	// - repeat until no more triangles can be added

	// Perform line-line intersection for texel against quad
	float2 texelOrigins[] =
	{
		texelTopLeftNDC,
		texelBottomLeftNDC,
		texelTopRightNDC,
		texelBottomRightNDC
	};

	float2 texelDirections[] =
	{
		texelTopRightNDC - texelTopLeftNDC,
		texelTopLeftNDC - texelBottomLeftNDC,
		texelBottomRightNDC - texelTopRightNDC,
		texelBottomLeftNDC - texelBottomRightNDC
	};

	float2 quadOrigins[] =
	{
		quadTopLeftNDC.xy,
		quadBottomLeftNDC.xy,
		quadTopRightNDC.xy,
		quadBottomRightNDC.xy
	};

	float2 quadDirections[] =
	{
		quadTopRightNDC.xy - quadTopLeftNDC.xy,
		quadTopLeftNDC.xy - quadBottomLeftNDC.xy,
		quadBottomRightNDC.xy - quadTopRightNDC.xy,
		quadBottomLeftNDC.xy - quadBottomRightNDC.xy
	};

	// The overlap area polygon of two quads can at most have 8 vertices
	float2 overlapVertices[8];
	uint overlapVertexCount = 0;

	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			float2 intersection;
			
			// Perform line segment intersection for all edges
			if (GetLineSegmentIntersection(texelOrigins[i], texelDirections[i], quadOrigins[j], quadDirections[j], intersection))
			{
				overlapVertices[overlapVertexCount] = intersection;
				overlapVertexCount++;
			}
		}

		// Perform vertex inside other quad check for all vertices
		if (IsInsideQuad(texelOrigins[i], quadOrigins[0], quadOrigins[1], quadOrigins[2], quadOrigins[3]))
		{
			overlapVertices[overlapVertexCount] = texelOrigins[i];
			overlapVertexCount++;
		}

		if (IsInsideQuad(quadOrigins[i], texelOrigins[0], texelOrigins[1], texelOrigins[2], texelOrigins[3]))
		{
			overlapVertices[overlapVertexCount] = quadOrigins[i];
			overlapVertexCount++;
		}
	}

	float2 overlapCenter = { 0, 0 };

	for (int i = 0; i < overlapVertexCount; i++)
	{
		overlapCenter += overlapVertices[i];

		if (distance(texelNDC, overlapVertices[i]) < vertexSizeNDC)
		{
			color.rgb += float3(1, 1, 1);
		}
	}

	if (overlapVertexCount > 0)
	{
		// Since the overlap polygon is convex, its center must lie within the polygon
		overlapCenter /= overlapVertexCount;

		if (distance(texelNDC, overlapCenter) < vertexSizeNDC)
		{
			color.rgb = float3(0.5f, 0.5f, 0.5f);
		}

		// Sort vertices clockwise around the center
		float2 overlapVerticesOrdered[8] = overlapVertices;
		float overlapVertexAngles[8] = { 2 * PI, 2 * PI, 2 * PI, 2 * PI, 2 * PI, 2 * PI, 2 * PI, 2 * PI };

		float2 edgeForAngle = overlapVerticesOrdered[0] - overlapCenter;
		overlapVerticesOrdered[0] = overlapVertices[0];
		overlapVertexAngles[0] = 0.0f;

		for (int i = 1; i < overlapVertexCount; i++)
		{
			overlapVertexAngles[i] = GetCounterclockwiseAngle(edgeForAngle, overlapVertices[i] - overlapCenter);
		}

		for (int j = 1; j < overlapVertexCount; j++)
		{
			int indexWithSmallestAngle = 1;

			for (int i = 2; i < overlapVertexCount; i++)
			{
				if (overlapVertexAngles[i] < overlapVertexAngles[indexWithSmallestAngle])
				{
					indexWithSmallestAngle = i;
				}
			}

			overlapVerticesOrdered[j] = overlapVertices[indexWithSmallestAngle];
			overlapVertexAngles[indexWithSmallestAngle] = 2 * PI;
		}

		// Perform triangulation
		for (int i = 0; i < overlapVertexCount - 1; i++)
		{
			if (IsInsideTriangle(texelNDC, overlapCenter, overlapVerticesOrdered[i], overlapVerticesOrdered[i + 1]))
			{
				color.rgb = float3((i + 1) / (float)overlapVertexCount, (i + 1) / (float)overlapVertexCount, 0);
			}
		}

		// Last triangle
		if (IsInsideTriangle(texelNDC, overlapCenter, overlapVerticesOrdered[overlapVertexCount - 1], overlapVerticesOrdered[0]))
		{
			color.rgb = float3(1, 1, 0);
		}
	}

	outputColorTexture[id.xy] = color;
	return;
#endif

#ifdef DEBUG_SINGLE_QUAD
	float3 pointPositionWorld = float3(0, -1, 1);
	float3 pointNormalWorld = float3(0, 1, 0);

	float4 pointPositionNDC = mul(mul(float4(pointPositionWorld, 1), View), Projection);
	pointPositionNDC = pointPositionNDC / pointPositionNDC.w;

	if (pointPositionNDC.z < 0)
	{
		outputColorTexture[id.xy] = float4(1, 0, 0, 1);
		return;
	}

	float3 viewDirection = normalize(pointPositionWorld - cameraPosition);
	float angle = acos(dot(pointNormalWorld, -viewDirection));

	if (angle > (PI / 2))
	{
		outputColorTexture[id.xy] = float4(0.5f, 0, 1, 1);
		return;
	}

	// Construct a quad in world space
	float3 cameraRight = float3(View[0][0], View[1][0], View[2][0]);
	float3 cameraUp = float3(View[0][1], View[1][1], View[2][1]);
	float3 cameraForward = float3(View[0][2], View[1][2], View[2][2]);

	float3 up;
	float3 right;

	// Quad should face in the same direction as the normal or it should be aligned with the camera
	if (orientedSplat)
	{
		up = 0.5f * splatSize * normalize(cross(pointNormalWorld, cameraRight));
		right = 0.5f * splatSize * normalize(cross(pointNormalWorld, up));
	}
	else
	{
		up = 0.5f * splatSize * cameraUp;
		right = 0.5f * splatSize * cameraRight;
	}

	float3 quadCenter = pointPositionWorld;
	float3 quadTopLeft = quadCenter + up - right;
	float3 quadTopRight = quadCenter + up + right;
	float3 quadBottomLeft = quadCenter - up - right;
	float3 quadBottomRight = quadCenter - up + right;

	// Project quad
	float4x4 VP = mul(View, Projection);
	float4 quadCenterNDC = mul(float4(quadCenter, 1), VP);
	float4 quadTopLeftNDC = mul(float4(quadTopLeft, 1), VP);
	float4 quadTopRightNDC = mul(float4(quadTopRight, 1), VP);
	float4 quadBottomLeftNDC = mul(float4(quadBottomLeft, 1), VP);
	float4 quadBottomRightNDC = mul(float4(quadBottomRight, 1), VP);

	// Homogeneous division
	quadCenterNDC /= quadCenterNDC.w;
	quadTopLeftNDC /= quadTopLeftNDC.w;
	quadTopRightNDC /= quadTopRightNDC.w;
	quadBottomLeftNDC /= quadBottomLeftNDC.w;
	quadBottomRightNDC /= quadBottomRightNDC.w;

	// Calculate edge normal vectors (pointing towards quad center)
	float2 quadLeftNormal = GetPerpendicularVector(quadTopLeftNDC.xy - quadBottomLeftNDC.xy);
	float2 quadTopNormal = GetPerpendicularVector(quadTopRightNDC.xy - quadTopLeftNDC.xy);
	float2 quadRightNormal = GetPerpendicularVector(quadBottomRightNDC.xy - quadTopRightNDC.xy);
	float2 quadBottomNormal = GetPerpendicularVector(quadBottomLeftNDC.xy - quadBottomRightNDC.xy);

	uint2 texel = uint2(id.x, id.y);

	// Calculate normalized device coordinates for the four texel corners
	float2 texelTopLeftNDC = resolutionPullPush * (texel / (float)resolutionOutput);
	texelTopLeftNDC = 2 * (texelTopLeftNDC / float2(resolutionX, resolutionY)) - 1;
	float2 texelTopRightNDC = resolutionPullPush * ((texel + uint2(1, 0)) / (float)resolutionOutput);
	texelTopRightNDC = 2 * (texelTopRightNDC / float2(resolutionX, resolutionY)) - 1;
	float2 texelBottomLeftNDC = resolutionPullPush * ((texel + uint2(0, 1)) / (float)resolutionOutput);
	texelBottomLeftNDC = 2 * (texelBottomLeftNDC / float2(resolutionX, resolutionY)) - 1;
	float2 texelBottomRightNDC = resolutionPullPush * ((texel + uint2(1, 1)) / (float)resolutionOutput);
	texelBottomRightNDC = 2 * (texelBottomRightNDC / float2(resolutionX, resolutionY)) - 1;

	// Need to invert y-coordinate for correct coordinate system
	texelTopLeftNDC.y = -texelTopLeftNDC.y;
	texelTopRightNDC.y = -texelTopRightNDC.y;
	texelBottomLeftNDC.y = -texelBottomLeftNDC.y;
	texelBottomRightNDC.y = -texelBottomRightNDC.y;

	// Only color the texel if all of its texel corners are inside the projected quad
	if (IsInsideQuad(texelTopLeftNDC, quadTopLeftNDC.xy, quadBottomRightNDC.xy, quadLeftNormal, quadTopNormal, quadRightNormal, quadBottomNormal)
		&& IsInsideQuad(texelTopRightNDC, quadTopLeftNDC.xy, quadBottomRightNDC.xy, quadLeftNormal, quadTopNormal, quadRightNormal, quadBottomNormal)
		&& IsInsideQuad(texelBottomLeftNDC, quadTopLeftNDC.xy, quadBottomRightNDC.xy, quadLeftNormal, quadTopNormal, quadRightNormal, quadBottomNormal)
		&& IsInsideQuad(texelBottomRightNDC, quadTopLeftNDC.xy, quadBottomRightNDC.xy, quadLeftNormal, quadTopNormal, quadRightNormal, quadBottomNormal))
	{
		outputColorTexture[texel] = float4(0, 1, 0, 1);
	}
	else
	{
		outputColorTexture[texel] = float4(0, 0, 0, 1);
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
			float3 cameraRight = float3(View[0][0], View[1][0], View[2][0]);
			float3 cameraUp = float3(View[0][1], View[1][1], View[2][1]);
			float3 cameraForward = float3(View[0][2], View[1][2], View[2][2]);

			uint2 texel = uint2(id.x, id.y);

			// Calculate normalized device coordinates for the four texel corners
			float2 texelTopLeftNDC = resolutionPullPush * (texel / (float)resolutionOutput);
			texelTopLeftNDC = 2 * (texelTopLeftNDC / float2(resolutionX, resolutionY)) - 1;
			float2 texelTopRightNDC = resolutionPullPush * ((texel + uint2(1, 0)) / (float)resolutionOutput);
			texelTopRightNDC = 2 * (texelTopRightNDC / float2(resolutionX, resolutionY)) - 1;
			float2 texelBottomLeftNDC = resolutionPullPush * ((texel + uint2(0, 1)) / (float)resolutionOutput);
			texelBottomLeftNDC = 2 * (texelBottomLeftNDC / float2(resolutionX, resolutionY)) - 1;
			float2 texelBottomRightNDC = resolutionPullPush * ((texel + uint2(1, 1)) / (float)resolutionOutput);
			texelBottomRightNDC = 2 * (texelBottomRightNDC / float2(resolutionX, resolutionY)) - 1;

			// Need to invert y-coordinate for correct coordinate system
			texelTopLeftNDC.y = -texelTopLeftNDC.y;
			texelTopRightNDC.y = -texelTopRightNDC.y;
			texelBottomLeftNDC.y = -texelBottomLeftNDC.y;
			texelBottomRightNDC.y = -texelBottomRightNDC.y;

			float smallestZ = farZ;

			outputColor = float4(0, 0, 0, 0);
			outputNormal = float4(0, 0, 0, 0);
			outputPosition = float4(0, 0, 0, 0);

			for (int y = 0; y < 2; y++)
			{
				for (int x = 0; x < 2; x++)
				{
					// Transform quad position and normal into world space
					float4 inputColor = inputColorTexture[2 * pixel + uint2(x, y)];
					float4 inputNormal = inputNormalTexture[2 * pixel + uint2(x, y)];
					float4 inputPosition = inputPositionTexture[2 * pixel + uint2(x, y)];

					float4 quadPositionLocal = mul(inputPosition, WorldViewProjectionInverse);
					quadPositionLocal /= quadPositionLocal.w;

					float3 quadNormalWorld = inputNormal.xyz;
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

					float3 quadCenter = quadPositionWorld;
					float3 quadTopLeft = quadCenter + quadUp - quadRight;
					float3 quadTopRight = quadCenter + quadUp + quadRight;
					float3 quadBottomLeft = quadCenter - quadUp - quadRight;
					float3 quadBottomRight = quadCenter - quadUp + quadRight;

					// Project quad
					float4x4 VP = mul(View, Projection);
					float4 quadCenterNDC = mul(float4(quadCenter, 1), VP);
					float4 quadTopLeftNDC = mul(float4(quadTopLeft, 1), VP);
					float4 quadTopRightNDC = mul(float4(quadTopRight, 1), VP);
					float4 quadBottomLeftNDC = mul(float4(quadBottomLeft, 1), VP);
					float4 quadBottomRightNDC = mul(float4(quadBottomRight, 1), VP);

					// Homogeneous division
					quadCenterNDC /= quadCenterNDC.w;
					quadTopLeftNDC /= quadTopLeftNDC.w;
					quadTopRightNDC /= quadTopRightNDC.w;
					quadBottomLeftNDC /= quadBottomLeftNDC.w;
					quadBottomRightNDC /= quadBottomRightNDC.w;

					// Calculate edge normal vectors (pointing towards quad center)
					float2 quadLeftNormal = GetPerpendicularVector(quadTopLeftNDC.xy - quadBottomLeftNDC.xy);
					float2 quadTopNormal = GetPerpendicularVector(quadTopRightNDC.xy - quadTopLeftNDC.xy);
					float2 quadRightNormal = GetPerpendicularVector(quadBottomRightNDC.xy - quadTopRightNDC.xy);
					float2 quadBottomNormal = GetPerpendicularVector(quadBottomLeftNDC.xy - quadBottomRightNDC.xy);

					if ((inputPosition.w > 0.0f)
						&& IsInsideQuad(texelTopLeftNDC, quadTopLeftNDC.xy, quadBottomRightNDC.xy, quadLeftNormal, quadTopNormal, quadRightNormal, quadBottomNormal)
						&& IsInsideQuad(texelTopRightNDC, quadTopLeftNDC.xy, quadBottomRightNDC.xy, quadLeftNormal, quadTopNormal, quadRightNormal, quadBottomNormal)
						&& IsInsideQuad(texelBottomLeftNDC, quadTopLeftNDC.xy, quadBottomRightNDC.xy, quadLeftNormal, quadTopNormal, quadRightNormal, quadBottomNormal)
						&& IsInsideQuad(texelBottomRightNDC, quadTopLeftNDC.xy, quadBottomRightNDC.xy, quadLeftNormal, quadTopNormal, quadRightNormal, quadBottomNormal))
					{
						// Blend splats together that are within a certain z-range to the closest surface or only keep the closest splat
						if (texelBlending)
						{
							float differenceZ = quadPositionView.z - smallestZ;

							// Splat is closer than the currently closest splat, assign it as the surface splat
							if (differenceZ < 0)
							{
								smallestZ = quadPositionView.z;
								outputNormal = inputNormal;
								outputPosition = inputPosition;

								// If out of blend range, assign new base color for blending
								if (differenceZ < -blendRange)
								{
									outputColor = inputColor;
								}
								else
								{
									// Accumulate weighted colors and weights
									float blendWeight = 1.0f;
									outputColor.rgb += blendWeight * inputColor.rgb;
									outputColor.w += blendWeight * inputColor.w;
								}
							}
							else if (differenceZ < blendRange)
							{
								// Splat is behind current surface splat but within blend range, so blend colors
								float blendWeight = 1.0f;
								outputColor.rgb += blendWeight * inputColor.rgb;
								outputColor.w += blendWeight * inputColor.w;
							}
						}
						else if (quadPositionView.z < smallestZ)
						{
							// Keep only the closest splat
							smallestZ = quadPositionView.z;
							outputColor = inputColor;
							outputNormal = inputNormal;
							outputPosition = inputPosition;
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
			if (outputPosition.w <= 0.0f)
			{
				outputColor = inputColor;
				outputNormal = inputNormal;
				outputPosition = inputPosition;
			}
			else
			{
				float4 inputPositionLocal = mul(inputPosition, WorldViewProjectionInverse);
				inputPositionLocal /= inputPositionLocal.w;

				float3 inputPositionWorld = mul(inputPositionLocal, World).xyz;
				float3 inputPositionView = mul(float4(inputPositionWorld, 1), View).xyz;

				float4 outputPositionLocal = mul(outputPosition, WorldViewProjectionInverse);
				outputPositionLocal /= outputPositionLocal.w;

				float3 outputPositionWorld = mul(outputPositionLocal, World).xyz;
				float3 outputPositionView = mul(float4(outputPositionWorld, 1), View).xyz;

				float differenceZ = inputPositionView.z - outputPositionView.z;

				if (differenceZ < 0)
				{
					outputNormal = inputNormal;
					outputPosition = inputPosition;

					if (!texelBlending || (differenceZ < -blendRange))
					{
						outputColor = inputColor;
					}
					else
					{
						float blendWeight = 1.0f;
						outputColor.rgb += blendWeight * inputColor.rgb;
						outputColor.w += blendWeight * inputColor.w;
					}
				}
				else if (texelBlending && (differenceZ < blendRange))
				{
					float blendWeight = 1.0f;
					outputColor.rgb += blendWeight * inputColor.rgb;
					outputColor.w += blendWeight * inputColor.w;
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