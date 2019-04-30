#include "Octree.hlsl"
#include "OctreeConstantBuffer.hlsl"

StructuredBuffer<OctreeNode> nodesBuffer : register(t0);
ConsumeStructuredBuffer<OctreeNodeTraversalEntry> inputConsumeBuffer : register(u0);
AppendStructuredBuffer<OctreeNodeTraversalEntry> outputAppendBuffer : register(u1);
AppendStructuredBuffer<OctreeNodeTraversalEntry> vertexAppendBuffer : register(u2);

[numthreads(1024, 1, 1)]
void CS (uint3 id : SV_DispatchThreadID)
{
    // Make sure that this thread is working on valid data
    if (id.x < inputCount)
    {
        // Get some entry that this thread has to check
		OctreeNodeTraversalEntry entry = inputConsumeBuffer.Consume();
        OctreeNode node = nodesBuffer[entry.index];

		// Get the childrenMask
		uint childrenMask = node.properties.childrenMaskAndWeights & 0xff;

		bool traverseChildren = true;
		bool insideViewFrustum = true;

		// View frustum culling, check if this node is fully inside the view frustum only when the parent isn't (the children of a node are always inside the view frustum then the node itself is inside it)
		if (!entry.parentInsideViewFrustum)
		{
			// Generate all the 6 planes of the view frustum
			float3 viewFrustumPlanes[6][2] =
			{
				{ localViewFrustumNearTopLeft, localViewPlaneNearNormal },			// Near Plane
				{ localViewFrustumFarBottomRight, localViewPlaneFarNormal },		// Far Plane
				{ localViewFrustumNearTopLeft, localViewPlaneLeftNormal },			// Left Plane
				{ localViewFrustumFarBottomRight, localViewPlaneRightNormal },		// Right Plane
				{ localViewFrustumNearTopLeft, localViewPlaneTopNormal },			// Top Plane
				{ localViewFrustumFarBottomRight, localViewPlaneBottomNormal }		// Bottom Plane
			};

			float extends = entry.size / 2.0f;

			float3 boundingCube[8] =
			{
				entry.position + float3(extends, extends, extends),
				entry.position + float3(-extends, extends, extends),
				entry.position + float3(extends, -extends, extends),
				entry.position + float3(extends, extends, -extends),
				entry.position + float3(-extends, -extends, extends),
				entry.position + float3(extends, -extends, -extends),
				entry.position + float3(-extends, extends, -extends),
				entry.position + float3(-extends, -extends, -extends),
			};

			int outsideCount = 0;
			int insideCount = 0;

			// Check if the bounding cube is fully inside, fully outside or overlapping the view frustum
			for (int i = 0; i < 8; i++)
			{
				// Check for each position if it is inside the view frustum
				bool positionIsInside = true;

				for (int j = 0; j < 6; j++)
				{
					// Compute the signed distance to each of the planes
					if (dot(boundingCube[i] - viewFrustumPlanes[j][0], viewFrustumPlanes[j][1]) > 0)
					{
						// The position cannot be fully inside the view frustum when it is on the wrong side of one of the 6 planes
						positionIsInside = false;
						break;
					}
				}

				if (positionIsInside)
				{
					insideCount++;
				}
				else
				{
					outsideCount++;
				}
			}

			if (outsideCount == 8)
			{
				// Handle the case that the bounding cube positions are not inside the view frustum but it is still overlapping (e.g. edge only intersection, cube contains view frustum)
				// Do ray sphere intersection for all the 12 view frustum rays against the sphere representation of the bounding cube (radius is half the diagonal)
				float3 rays[12][2] =
				{
					{ localViewFrustumNearTopRight, localViewFrustumNearTopLeft },
					{ localViewFrustumNearBottomRight, localViewFrustumNearBottomLeft },
					{ localViewFrustumNearBottomLeft, localViewFrustumNearTopLeft },
					{ localViewFrustumNearBottomRight, localViewFrustumNearTopRight },
					{ localViewFrustumFarTopRight, localViewFrustumFarTopLeft },
					{ localViewFrustumFarBottomRight, localViewFrustumFarBottomLeft },
					{ localViewFrustumFarBottomLeft, localViewFrustumFarTopLeft },
					{ localViewFrustumFarBottomRight, localViewFrustumFarTopRight },
					{ localViewFrustumNearTopLeft, localViewFrustumFarTopLeft },
					{ localViewFrustumNearTopRight, localViewFrustumFarTopRight },
					{ localViewFrustumNearBottomLeft, localViewFrustumFarBottomLeft },
					{ localViewFrustumNearBottomRight, localViewFrustumFarBottomRight }
				};

				// Create a sphere that encloses the bounding cube to test against
				bool intersects = false;
				float3 c = entry.position;
				float r = length(float3(extends, extends, extends));

				for (int i = 0; i < 12; i++)
				{
					float3 o = rays[i][0];
					float3 v = rays[i][1] - rays[i][0];

					// Store the length of the frustum line and normalize it
					float l = length(v);
					v /= l;

					// Calculate the factor under the square root
					float3 oMinusC = o - c;
					float vDotOMinusC = dot(v, oMinusC);
					float f = vDotOMinusC * vDotOMinusC - (oMinusC.x * oMinusC.x + oMinusC.y * oMinusC.y + oMinusC.z * oMinusC.z - r * r);

					if (f > 0)
					{
						// Intersecting the sphere at two points, calculate whether this point is on the line of the view frustum
						float s = sqrt(f);
						float d1 = -vDotOMinusC + s;
						float d2 = -vDotOMinusC - s;

						if ((d1 >= 0 && d1 <= l) || (d2 >= 0 && d2 <= l))
						{
							// Interecting, check children again
							insideViewFrustum = false;
							intersects = true;
							break;
						}
					}
				}

				if (!intersects)
				{
					// The whole cube is outside, don't add it or any of its children
					return;
				}
			}
			else if (insideCount != 0 && outsideCount != 0)
			{
				// Some positions are outside and some are inside the view frustum, check the children again
				insideViewFrustum = false;
			}

			// Otherwise the cube is fully inside the view frustum as assumed
		}

		// Check if only to return the vertices at the given level
		if (level >= 0)
		{
			if (entry.depth == level)
			{
				// Draw this vertex and don't traverse further
				traverseChildren = false;
				vertexAppendBuffer.Append(entry);
			}
		}
		else
		{
			// Calculate required splat size
			float distanceToCamera = distance(localCameraPosition, entry.position);
			float requiredSplatSize = splatSize * (2.0f * tan(fovAngleY / 2.0f)) * distanceToCamera;

			// Check against required splat size and draw this vertex if it is smaller
			if (entry.size < requiredSplatSize || childrenMask == 0)
			{
				traverseChildren = false;
				vertexAppendBuffer.Append(entry);
			}
		}

        if (traverseChildren)
        {
			float childExtend = 0.25f * entry.size;

			float3 childPositions[8] =
			{
				entry.position + float3(childExtend, childExtend, childExtend),
				entry.position + float3(childExtend, childExtend, -childExtend),
				entry.position + float3(childExtend, -childExtend, childExtend),
				entry.position + float3(childExtend, -childExtend, -childExtend),
				entry.position + float3(-childExtend, childExtend, childExtend),
				entry.position + float3(-childExtend, childExtend, -childExtend),
				entry.position + float3(-childExtend, -childExtend, childExtend),
				entry.position + float3(-childExtend, -childExtend, -childExtend)
			};

			uint count = 0;

            // Check all the children in the next compute shader iteration
            for (int i = 0; i < 8; i++)
            {
				if (childrenMask & (1 << i))
				{
					OctreeNodeTraversalEntry childEntry;
					childEntry.index = node.childrenStartOrLeafPositionFactors + count;
					childEntry.position = childPositions[i];
					childEntry.size = entry.size * 0.5f;
					childEntry.parentInsideViewFrustum = insideViewFrustum;
					childEntry.depth = entry.depth + 1;

					outputAppendBuffer.Append(childEntry);

					count++;
				}
            }
        }
    }
}