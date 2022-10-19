#include "GroundTruth.hlsl"

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
	int resolutionX;
	int resolutionY;
//------------------------------------------------------------------------------ (16 byte boundary)
	int resolutionOutput;
	float splatSize;
	int pullPushLevel;
	// 4 bytes auto paddding
//------------------------------------------------------------------------------ (16 byte boundary)
};  // Total: 32 bytes with constant buffer packing rules

bool IsInsideEllipse(float2 position, float2 ellipseCenter, float ellipseSemiMinorAxis, float ellipseSemiMajorAxis)
{
	float2 difference = position - ellipseCenter;
	float2 differenceSquared = difference * difference;
	float minorAxisSquared = ellipseSemiMinorAxis * ellipseSemiMinorAxis;
	float majorAxisSquared = ellipseSemiMajorAxis * ellipseSemiMajorAxis;

	return distance(position, ellipseCenter) < ellipseSemiMajorAxis;

	return ((differenceSquared.x / majorAxisSquared) + (differenceSquared.y / minorAxisSquared)) <= 1.0f;
}

[numthreads(32, 32, 1)]
void CS(uint3 id : SV_DispatchThreadID)
{
	if (debug)
	{
		outputColorTexture[id.xy] = inputColorTexture.SampleLevel(samplerState, (id.xy + float2(0.5f, 0.5f)) / resolutionOutput, 0);
		return;
	}

/////////////////////////////////////////////////////
// TESTING

	float3 pointNormalLocal = float3(0, 0, 1);
	float3 pointPositionLocal = float3(0, 0, 0);
	float3 pointPositionWorld = mul(float4(pointPositionLocal, 1), World).xyz;
	float4 pointPositionNDC = mul(mul(float4(pointPositionWorld, 1), View), Projection);
	pointPositionNDC = pointPositionNDC / pointPositionNDC.w;

	float3 pointNormalWorld = normalize(mul(float4(pointNormalLocal, 0), WorldInverseTranspose).xyz);

	// Construct splat size scaled vectors that are perpendicular to the normal and each other
	float3 cameraRight = float3(View[0][0], View[1][0], View[2][0]);
	float3 cameraUp = float3(View[0][1], View[1][1], View[2][1]);
	float3 cameraForward = float3(View[0][2], View[1][2], View[2][2]);

	float splatSizeWorld = length(mul(float4(splatSize, 0, 0, 0), World).xyz);
	float3 up = 0.5f * splatSizeWorld * normalize(cross(pointNormalWorld, cameraRight));
	float3 right = 0.5f * splatSizeWorld * normalize(cross(pointNormalWorld, up));

	// Project the splat center and both splat plane vectors
	float4 upNDC = mul(mul(float4(pointPositionWorld + up, 1), View), Projection);
	float4 rightNDC = mul(mul(float4(pointPositionWorld + right, 1), View), Projection);
	upNDC = upNDC / upNDC.w;
	rightNDC = rightNDC / rightNDC.w;

	upNDC = upNDC - pointPositionNDC;
	rightNDC = rightNDC - pointPositionNDC;

	float rightNDCLength = length(rightNDC.xy);
	float upNDCLength = length(upNDC.xy);

	float2 semiMinorAxis;
	float2 semiMajorAxis;
	
	if (rightNDCLength < upNDCLength)
	{
		semiMinorAxis = rightNDC.xy;
		semiMajorAxis = upNDC.xy;
	}
	else
	{
		semiMinorAxis = upNDC.xy;
		semiMajorAxis = rightNDC.xy;
	}

	float angleToMinor = -dot(semiMinorAxis, float2(0, 1)) / length(semiMinorAxis);
	float2x2 rotationToMinor = { cos(angleToMinor), sin(angleToMinor), -sin(angleToMinor), cos(angleToMinor) };

	// TODO: Rotate pixel such that minor axis aligns with x-axis
	int resolutionFull = pow(2, ceil(log2(max(resolutionX, resolutionY))));

	float2 absoluteTopLeft = resolutionFull * (id.xy / (float)resolutionOutput);
	float2 ndcTopLeft = 2 * (absoluteTopLeft / float2(resolutionX, resolutionY)) - 1;

	float2 absoluteTopRight = resolutionFull * ((id.xy + uint2(1, 0)) / (float)resolutionOutput);
	float2 ndcTopRight = 2 * (absoluteTopRight / float2(resolutionX, resolutionY)) - 1;

	float2 absoluteBottomLeft = resolutionFull * ((id.xy + uint2(0, 1)) / (float)resolutionOutput);
	float2 ndcBottomLeft = 2 * (absoluteBottomLeft / float2(resolutionX, resolutionY)) - 1;

	float2 absoluteBottomRight = resolutionFull * ((id.xy + uint2(1, 1)) / (float)resolutionOutput);
	float2 ndcBottomRight = 2 * (absoluteBottomRight / float2(resolutionX, resolutionY)) - 1;

	//// Apply rotation
	//ndcTopLeft -= pointPositionNDC.xy;
	//ndcTopLeft = mul(ndcTopLeft, rotationToMinor);
	//ndcTopLeft += pointPositionNDC.xy;

	if (IsInsideEllipse(ndcTopLeft, pointPositionNDC.xy, length(semiMinorAxis), length(semiMajorAxis)))
	{
		outputColorTexture[id.xy] = float4(1, 0, 0, 1);
	}
	else
	{
		outputColorTexture[id.xy] = float4(0, 0, 0, 1);
	}

	return;

// TESTING
////////////////////////////////////////////////////

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
					outputPosition.z = depth;
					outputPosition.w = 1.0f;
				}
			}
		}
		else
		{
			int resolutionFull = pow(2, ceil(log2(max(resolutionX, resolutionY))));

			// Compute NDC coordinates of the 4 corners of the output texel
			float2 outputTexelTopLeft = resolutionFull * (pixel / (float)resolutionOutput);
			outputTexelTopLeft /= float2(resolutionX, resolutionY);
			outputTexelTopLeft = (2 * outputTexelTopLeft) - 1;

			float2 outputTexelTopRight = resolutionFull * ((pixel + uint2(1, 0)) / (float)resolutionOutput);
			outputTexelTopRight /= float2(resolutionX, resolutionY);
			outputTexelTopRight = (2 * outputTexelTopRight) - 1;

			float2 outputTexelBottomLeft = resolutionFull * ((pixel + uint2(0, 1)) / (float)resolutionOutput);
			outputTexelBottomLeft /= float2(resolutionX, resolutionY);
			outputTexelBottomLeft = (2 * outputTexelBottomLeft) - 1;

			float2 outputTexelBottomRight = resolutionFull * ((pixel + uint2(1, 1)) / (float)resolutionOutput);
			outputTexelBottomRight /= float2(resolutionX, resolutionY);
			outputTexelBottomRight = (2 * outputTexelBottomRight) - 1;

			float4 cameraPosition = float4(0, 0, 0, 1);
			cameraPosition = mul(cameraPosition, WorldViewProjectionInverse);
			cameraPosition = cameraPosition / cameraPosition.w;

			float smallestDepth = 1.0f;

			outputColor = float4(0, 0, 0, 0);
			outputNormal = float4(0, 0, 0, 0);
			outputPosition = float4(0, 0, 0, 0);

			for (int y = 0; y < 2; y++)
			{
				for (int x = 0; x < 2; x++)
				{
					float4 inputColor = inputColorTexture[2 * pixel + uint2(x, y)];
					float4 inputPosition = inputPositionTexture[2 * pixel + uint2(x, y)];

					float4 inputPositionLocal = mul(inputPosition, WorldViewProjectionInverse);
					inputPositionLocal = inputPositionLocal / inputPositionLocal.w;

					float distanceInputToCamera = distance(cameraPosition, inputPositionLocal);
					float projectedSplatSizeInput = splatSize / ((2.0f * tan(fovAngleY / 2.0f)) * distanceInputToCamera);

					float distanceTopLeft = distance(outputTexelTopLeft, inputPosition.xy);
					float distanceTopRight = distance(outputTexelTopRight, inputPosition.xy);
					float distanceBottomLeft = distance(outputTexelBottomLeft, inputPosition.xy);
					float distanceBottomRight = distance(outputTexelBottomRight, inputPosition.xy);

					if ((inputPosition.w > 0.0f) && (inputPosition.z < smallestDepth) && (distanceTopLeft < projectedSplatSizeInput) && (distanceTopRight < projectedSplatSizeInput) && (distanceBottomLeft < projectedSplatSizeInput) && (distanceBottomRight < projectedSplatSizeInput))
					{
						smallestDepth = inputPosition.z;
						outputColor = inputColor;
						outputPosition = inputPosition;
					}
				}
			}
		}
	}
	else
	{
		float2 uv = (pixel + float2(0.5f, 0.5f)) / resolutionOutput;
		float4 inputColor = inputColorTexture.SampleLevel(samplerState, uv, 0);
		float4 inputPosition = inputPositionTexture[pixel / 2];
		
		outputColor = outputColorTexture[pixel];
		outputPosition = outputPositionTexture[pixel];

		// Replace pixels where no point has been rendered to (therefore 4th component is zero) or pixels that are obscured by a closer surface from the higher level pull texture
		if ((inputPosition.w >= 1.0f) && ((outputPosition.w <= 0.0f) || (inputPosition.z < outputPosition.z)))
		{
			outputColor = inputColor;
			outputPosition = inputPosition;
		}
	}

	outputColorTexture[pixel] = outputColor;
	outputPositionTexture[pixel] = outputPosition;
}