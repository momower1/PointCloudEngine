#include "OctreeNode.h"

PointCloudEngine::OctreeNode::OctreeNode()
{
    // Default constructor used for parsing from file
}

PointCloudEngine::OctreeNode::OctreeNode(std::queue<OctreeNodeCreationEntry> &nodeCreationQueue, std::vector<OctreeNode> &nodes, std::vector<UINT>& children, const OctreeNodeCreationEntry &entry)
{
    size_t vertexCount = entry.vertices.size();
    
    if (vertexCount == 0)
    {
		ERROR_MESSAGE(L"Cannot create " + NAMEOF(OctreeNode) + L" from 0 " + NAMEOF(vertexCount));
        return;
    }

    // The octree is generated by fitting the vertices into a cube at the center position
    // Then this cube is splitted into 8 smaller child cubes along the center
    // For each child cube the octree generation is repeated
    // Assign this child to its parent
    if (entry.nodesIndex != UINT_MAX && entry.childrenIndex != UINT_MAX)
    {
		children[entry.childrenIndex] = entry.nodesIndex;
    }

    // Apply the k-means clustering algorithm to find clusters for the normals
    Vector3 means[4];
    const int k = min(vertexCount, 4);
    UINT verticesPerMean[4] = { 0, 0, 0, 0 };

    // Set initial means to the first k normals
    for (int i = 0; i < k; i++)
    {
        means[i] = entry.vertices[i].normal;
        verticesPerMean[i] = 1;
    }

    // Save the index of the mean that each vertex is assigned to
    bool meanChanged = true;
    byte *clusters = new byte[vertexCount];
    ZeroMemory(clusters, sizeof(byte) * vertexCount);

    while (meanChanged)
    {
        // Assign all the vertices to the closest mean to them
        for (UINT i = 0; i < vertexCount; i++)
        {
            float minDistance = Vector3::Distance(entry.vertices[i].normal, means[clusters[i]]);

            for (UINT j = 0; j < k; j++)
            {
                float distance = Vector3::Distance(entry.vertices[i].normal, means[j]);

                if (distance < minDistance)
                {
                    clusters[i] = j;
                    minDistance = distance;
                }
            }
        }

        // Calculate the new means from the vertices in each cluster
        Vector3 newMeans[4];

        for (int i = 0; i < k; i++)
        {
            verticesPerMean[i] = 0;
        }

        for (UINT i = 0; i < vertexCount; i++)
        {
            newMeans[clusters[i]] += entry.vertices[i].normal;
            verticesPerMean[clusters[i]] += 1;
        }

        meanChanged = false;

        // Update the means
        for (int i = 0; i < k; i++)
        {
            if (verticesPerMean[i] > 0)
            {
                newMeans[i] /= verticesPerMean[i];

                if (Vector3::DistanceSquared(means[i], newMeans[i]) > FLT_EPSILON)
                {
                    meanChanged = true;
                }

                means[i] = newMeans[i];
            }
        }

    }

	// Normalize the means
	for (UINT i = 0; i < k; i++)
	{
		means[i].Normalize();
	}

    // Initialize average colors that are calculated per cluster
	float normalCones[4] = { 0, 0, 0, 0 };
    double averageReds[4] = { 0, 0, 0, 0 };
    double averageGreens[4] = { 0, 0, 0, 0 };
    double averageBlues[4] = { 0, 0, 0, 0 };

    // Calculate color
    for (UINT i = 0; i < vertexCount; i++)
    {
        averageReds[clusters[i]] += entry.vertices[i].color[0];
        averageGreens[clusters[i]] += entry.vertices[i].color[1];
        averageBlues[clusters[i]] += entry.vertices[i].color[2];

		// Calculate the angle in [0, pi] between the mean normal and this vertex normal
		float angle = acos(means[clusters[i]].Dot(entry.vertices[i].normal));

		// Save the maximum angle to any of the vertices in the cluster as normal cone
		normalCones[clusters[i]] = max(normalCones[clusters[i]], angle);
    }

	delete[] clusters;

    // Assign node properties
	properties.childrenMask = 0;

    for (int i = 0; i < 4; i++)
    {
        if (verticesPerMean[i] > 0)
        {
            averageReds[i] /= verticesPerMean[i];
            averageGreens[i] /= verticesPerMean[i];
            averageBlues[i] /= verticesPerMean[i];

            properties.normals[i] = ClusterNormal(means[i], normalCones[i]);
            properties.colors[i] = Color16(averageReds[i], averageGreens[i], averageBlues[i]);
        }
    }

	// Assign weights (one of the 4 can be omitted because the sum is always 100%)
	for (int i = 0; i < 3; i++)
	{
		properties.weights[i] = (255.0f * verticesPerMean[i]) / vertexCount;
	}

    // Only subdivide further when this is not a leaf node and the max octree depth is not met yet
    if ((vertexCount > 1) && (entry.depth < settings->maxOctreeDepth))
    {
		// Split and create children vertices
		std::vector<Vertex> childVertices[8];

		// Fit each vertex into its corresponding child cube
		for (auto it = entry.vertices.begin(); it != entry.vertices.end(); it++)
		{
			Vertex v = *it;

			if (v.position.x > entry.position.x)
			{
				if (v.position.y > entry.position.y)
				{
					if (v.position.z > entry.position.z)
					{
						childVertices[0].push_back(v);
					}
					else
					{
						childVertices[1].push_back(v);
					}
				}
				else
				{
					if (v.position.z > entry.position.z)
					{
						childVertices[2].push_back(v);
					}
					else
					{
						childVertices[3].push_back(v);
					}
				}
			}
			else
			{
				if (v.position.y > entry.position.y)
				{
					if (v.position.z > entry.position.z)
					{
						childVertices[4].push_back(v);
					}
					else
					{
						childVertices[5].push_back(v);
					}
				}
				else
				{
					if (v.position.z > entry.position.z)
					{
						childVertices[6].push_back(v);
					}
					else
					{
						childVertices[7].push_back(v);
					}
				}
			}
		}

		// Store the start index of the children in the children array
		childrenStartOrLeafPositionFactors = children.size();

        for (int i = 0; i < 8; i++)
        {
            if (childVertices[i].size() > 0)
            {
                // Add a new entry to the queue
                OctreeNodeCreationEntry childEntry;
                childEntry.nodesIndex = UINT_MAX;
				childEntry.childrenIndex = children.size();
                childEntry.vertices = childVertices[i];
                childEntry.position = GetChildPosition(entry.position, entry.size, i);
                childEntry.size = entry.size * 0.5f;
                childEntry.depth = entry.depth + 1;

				// Add this entry to the mask
				properties.childrenMask |= 1 << i;

				// Reserve a spot for the children index that will be assigned later
				children.push_back(UINT_MAX);

				// Add this to the queue
                nodeCreationQueue.push(childEntry);
            }
        }
    }
	else
	{
		// This is a leaf node with childrenMask=0 representing exactly one or more vertices
		// The bounding cube can be much larger than the vertices that it represents -> the bounding cube position does not represent the vertex positions well
		// Idea: store factors from the average vertex position in the childrenStartOrLeafPositionFactors to representing a more accurate position
		// Each 8 bits store the distance factor from the smallest position of the bounding cube in respect to the size of the cube in each axis (x, y, z)
		childrenStartOrLeafPositionFactors = 0;

		// Compute the average position of the vertices contained in this leaf node
		Vector3 averagePosition = Vector3::Zero;

		for (UINT i = 0; i < vertexCount; i++)
		{
			averagePosition += entry.vertices[i].position;
		}

		averagePosition /= vertexCount;

		// Use the offset from the smallest position of the bounding cube to compute the factors
		Vector3 offset = averagePosition - (entry.position - (0.5f * entry.size * Vector3::One));

		float factorX = offset.x / entry.size;
		float factorY = offset.y / entry.size;
		float factorZ = offset.z / entry.size;

		// Store all of them in the 32bit uint
		childrenStartOrLeafPositionFactors |= static_cast<UINT>(0xff * factorX) << 16;
		childrenStartOrLeafPositionFactors |= static_cast<UINT>(0xff * factorY) << 8;
		childrenStartOrLeafPositionFactors |= static_cast<UINT>(0xff * factorZ);
	}
}

void PointCloudEngine::OctreeNode::GetVertices(const std::vector<OctreeNode>& nodes, std::queue<OctreeNodeTraversalEntry> &nodesQueue, std::vector<OctreeNodeVertex> &octreeVertices, const OctreeNodeTraversalEntry &entry, const OctreeConstantBuffer &octreeConstantBufferData) const
{
	bool visible = false;
	bool insideViewFrustum = true;
	bool traverseChildren = true;

	if (octreeConstantBufferData.useCulling)
	{
		// Backface culling by comparing the maximum angle (normal cone) from the mean to all normals in the cluster against the view direction
		OctreeNode node = nodes[entry.index];
		Vector3 localViewDirection = entry.position - octreeConstantBufferData.localCameraPosition;
		localViewDirection.Normalize();

		// Calculate the angle between the view direction, camera forward vector and each normal
		for (int i = 0; i < 4; i++)
		{
			ClusterNormal clusterNormal = node.properties.normals[i];
			Vector3 normal = clusterNormal.GetVector3();
			float cone = clusterNormal.GetCone();

			// Also check against the camera forward vector since the node position can yield a heavily different view direction
			float firstAngle = acos(normal.Dot(-localViewDirection));
			float secondAngle = acos(normal.Dot(octreeConstantBufferData.localViewPlaneNearNormal));
			float angle = min(firstAngle, secondAngle);

			// At the edge of the cone the angle can be up to pi/2 larger for the node to be still visible
			if (angle < (XM_PI / 2) + cone)
			{
				visible = true;
				break;
			}
		}

		if (!visible)
		{
			// The node and all of its children face away from the camera, don't draw it or traverse further
			return;
		}

		// View frustum culling, check if this node is fully inside the view frustum only when the parent isn't (the children of a node are always inside the view frustum then the node itself is inside it)
		if (!entry.parentInsideViewFrustum)
		{
			// Generate all the 6 planes of the view frustum
			Plane viewFrustumPlanes[6] =
			{
				Plane(octreeConstantBufferData.localViewFrustumNearTopLeft, octreeConstantBufferData.localViewPlaneNearNormal),			// Near Plane
				Plane(octreeConstantBufferData.localViewFrustumFarBottomRight, octreeConstantBufferData.localViewPlaneFarNormal),		// Far Plane
				Plane(octreeConstantBufferData.localViewFrustumNearTopLeft, octreeConstantBufferData.localViewPlaneLeftNormal),			// Left Plane
				Plane(octreeConstantBufferData.localViewFrustumFarBottomRight, octreeConstantBufferData.localViewPlaneRightNormal),		// Right Plane
				Plane(octreeConstantBufferData.localViewFrustumNearTopLeft, octreeConstantBufferData.localViewPlaneTopNormal),			// Top Plane
				Plane(octreeConstantBufferData.localViewFrustumFarBottomRight, octreeConstantBufferData.localViewPlaneBottomNormal)		// Bottom Plane
			};

			float extends = entry.size / 2.0f;

			Vector3 boundingCube[8] =
			{
				entry.position + Vector3(extends, extends, extends),
				entry.position + Vector3(-extends, extends, extends),
				entry.position + Vector3(extends, -extends, extends),
				entry.position + Vector3(extends, extends, -extends),
				entry.position + Vector3(-extends, -extends, extends),
				entry.position + Vector3(extends, -extends, -extends),
				entry.position + Vector3(-extends, extends, -extends),
				entry.position + Vector3(-extends, -extends, -extends),
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
					if (viewFrustumPlanes[j].DotCoordinate(boundingCube[i]) > 0)
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
				Vector3 rays[12][2] =
				{
					{ octreeConstantBufferData.localViewFrustumNearTopRight, octreeConstantBufferData.localViewFrustumNearTopLeft },
					{ octreeConstantBufferData.localViewFrustumNearBottomRight, octreeConstantBufferData.localViewFrustumNearBottomLeft },
					{ octreeConstantBufferData.localViewFrustumNearBottomLeft, octreeConstantBufferData.localViewFrustumNearTopLeft },
					{ octreeConstantBufferData.localViewFrustumNearBottomRight, octreeConstantBufferData.localViewFrustumNearTopRight },
					{ octreeConstantBufferData.localViewFrustumFarTopRight, octreeConstantBufferData.localViewFrustumFarTopLeft },
					{ octreeConstantBufferData.localViewFrustumFarBottomRight, octreeConstantBufferData.localViewFrustumFarBottomLeft },
					{ octreeConstantBufferData.localViewFrustumFarBottomLeft, octreeConstantBufferData.localViewFrustumFarTopLeft },
					{ octreeConstantBufferData.localViewFrustumFarBottomRight, octreeConstantBufferData.localViewFrustumFarTopRight },
					{ octreeConstantBufferData.localViewFrustumNearTopLeft, octreeConstantBufferData.localViewFrustumFarTopLeft },
					{ octreeConstantBufferData.localViewFrustumNearTopRight, octreeConstantBufferData.localViewFrustumFarTopRight },
					{ octreeConstantBufferData.localViewFrustumNearBottomLeft, octreeConstantBufferData.localViewFrustumFarBottomLeft },
					{ octreeConstantBufferData.localViewFrustumNearBottomRight, octreeConstantBufferData.localViewFrustumFarBottomRight }
				};

				// Create a sphere that encloses the bounding cube to test against
				bool intersects = false;
				Vector3 c = entry.position;
				float r = Vector3(extends, extends, extends).Length();

				for (int i = 0; i < 12; i++)
				{
					Vector3 o = rays[i][0];
					Vector3 v = rays[i][1] - rays[i][0];

					// Store the length of the frustum line and normalize it
					float l = v.Length();
					v /= l;

					// Calculate the factor under the square root
					float vDotOMinusC = v.Dot(o - c);
					float f = vDotOMinusC * vDotOMinusC - ((o - c).LengthSquared() - r * r);

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
	}

	// Check if only to return the vertices at the given level
	if (octreeConstantBufferData.level >= 0)
	{
		if (entry.depth == octreeConstantBufferData.level)
		{
			// Draw this vertex and don't traverse further
			traverseChildren = false;
			octreeVertices.push_back(GetVertexFromTraversalEntry(entry));
		}
	}
	else
	{
		// Only return the vertices that have a projected size smaller than the required splat size or it is a leaf node
		float distanceToCamera = Vector3::Distance(octreeConstantBufferData.localCameraPosition, entry.position);

		// Scale the local space splat size by the fov and camera distance (result: size at that distance in local space)
		float requiredSplatSize = octreeConstantBufferData.splatResolution * (2.0f * tan(octreeConstantBufferData.fovAngleY / 2.0f)) * distanceToCamera;

		if ((entry.size < requiredSplatSize) || IsLeafNode())
		{
			// Draw this vertex, don't traverse further
			traverseChildren = false;
			octreeVertices.push_back(GetVertexFromTraversalEntry(entry));
		}
	}

	if (traverseChildren)
	{
		size_t count = 0;

		// Traverse the children
		for (int i = 0; i < 8; i++)
		{
			// Check if this child exists and add it to the queue
			if (properties.childrenMask & (1 << i))
			{
				OctreeNodeTraversalEntry childEntry;
				childEntry.index = childrenStartOrLeafPositionFactors + count;
				childEntry.position = GetChildPosition(entry.position, entry.size, i);
				childEntry.size = entry.size * 0.5f;
				childEntry.parentInsideViewFrustum = insideViewFrustum;
				childEntry.depth = entry.depth + 1;

				nodesQueue.push(childEntry);

				count++;
			}
		}
	}
}

bool PointCloudEngine::OctreeNode::IsLeafNode() const
{
	return (properties.childrenMask == 0);
}

Vector3 PointCloudEngine::OctreeNode::GetChildPosition(const Vector3& parentPosition, const float& parentSize, int childIndex) const
{
	/*
	Vector3 childPositions[8] =
	{
		parentPosition + Vector3(childExtend, childExtend, childExtend),		// 000
		parentPosition + Vector3(childExtend, childExtend, -childExtend),		// 001
		parentPosition + Vector3(childExtend, -childExtend, childExtend),		// 010
		parentPosition + Vector3(childExtend, -childExtend, -childExtend),		// 011
		parentPosition + Vector3(-childExtend, childExtend, childExtend),		// 100
		parentPosition + Vector3(-childExtend, childExtend, -childExtend),		// 101
		parentPosition + Vector3(-childExtend, -childExtend, childExtend),		// 110
		parentPosition + Vector3(-childExtend, -childExtend, -childExtend)		// 111
	};
	*/

	float childExtend = 0.25f * parentSize;

	return Vector3
	(
		parentPosition.x + ((childIndex & 0x4) ? -childExtend : childExtend),
		parentPosition.y + ((childIndex & 0x2) ? -childExtend : childExtend),
		parentPosition.z + ((childIndex & 0x1) ? -childExtend : childExtend)
	);
}

OctreeNodeVertex PointCloudEngine::OctreeNode::GetVertexFromTraversalEntry(const OctreeNodeTraversalEntry& entry) const
{
	OctreeNodeVertex vertex;

	// Check if this is a leaf node and therefore the position is stored more accurately in childrenStartOrLeafPositionFactors
	if (IsLeafNode())
	{
		// Extract the factors from childrenStartOrLeafPositionFactors and compute the more accurate position
		Vector3 startPosition = entry.position - (0.5f * entry.size * Vector3::One);

		// Get the factors from the 32bit uint
		float factorX = ((childrenStartOrLeafPositionFactors >> 16) & 0xff) / 255.0f;
		float factorY = ((childrenStartOrLeafPositionFactors >> 8) & 0xff) / 255.0f;
		float factorZ = (childrenStartOrLeafPositionFactors & 0xff) / 255.0f;

		vertex.position = startPosition + entry.size * Vector3(factorX, factorY, factorZ);
	}
	else
	{
		vertex.position = entry.position;
	}

	vertex.properties = properties;
	vertex.size = entry.size;

	return vertex;
}
