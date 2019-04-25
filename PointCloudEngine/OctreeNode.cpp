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

    // Initialize average colors that are calculated per cluster
    double averageReds[4] = { 0, 0, 0, 0 };
    double averageGreens[4] = { 0, 0, 0, 0 };
    double averageBlues[4] = { 0, 0, 0, 0 };

    // Calculate color
    for (UINT i = 0; i < vertexCount; i++)
    {
        averageReds[clusters[i]] += entry.vertices[i].color[0];
        averageGreens[clusters[i]] += entry.vertices[i].color[1];
        averageBlues[clusters[i]] += entry.vertices[i].color[2];
    }

    // Assign node properties
    for (int i = 0; i < 4; i++)
    {
		properties.weights[i] = 0;

        if (verticesPerMean[i] > 0)
        {
            averageReds[i] /= verticesPerMean[i];
            averageGreens[i] /= verticesPerMean[i];
            averageBlues[i] /= verticesPerMean[i];

            properties.normals[i] = PolarNormal(means[i]);
            properties.colors[i] = Color16(averageReds[i], averageGreens[i], averageBlues[i]);
			properties.weights[i] = (255.0f * verticesPerMean[i]) / vertexCount;
        }
    }

    delete[] clusters;

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

    // Only subdivide further if the size is above the minimum size
    if (entry.depth > 0)
    {
		childrenMask = 0;
		childrenStart = children.size();

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
                childEntry.depth = entry.depth - 1;

				// Add this entry to the mask
				childrenMask |= 1 << i;

				// Reserve a spot for the children index that will be assigned later
				children.push_back(UINT_MAX);

				// Add this to the queue
                nodeCreationQueue.push(childEntry);
            }
        }
    }
}

void PointCloudEngine::OctreeNode::GetVertices(std::queue<OctreeNodeTraversalEntry> &nodesQueue, std::vector<OctreeNodeVertex> &octreeVertices, const std::vector<UINT>& children, const OctreeNodeTraversalEntry &entry, const Vector3 &localCameraPosition, const float &splatSize) const
{
    // TODO: View frustum culling by checking the node bounding box against all the view frustum planes (don't check again if fully inside)
    // TODO: Visibility culling by comparing the maximum angle (normal cone) from the mean to all normals in the cluster against the view direction
    // Only return a vertex if its projected size is smaller than the passed size or it is a leaf node
    float distanceToCamera = Vector3::Distance(localCameraPosition, entry.position);

    // Scale the local space splat size by the fov and camera distance (Result: size at that distance in local space)
    float requiredSplatSize = splatSize * (2.0f * tan(settings->fovAngleY / 2.0f)) * distanceToCamera;

    if ((entry.size < requiredSplatSize) || IsLeafNode())
    {
        // Draw this vertex
        octreeVertices.push_back(GetVertexFromTraversalEntry(entry));
    }
    else
    {
		size_t count = 0;

		// Traverse the children
		for (int i = 0; i < 8; i++)
		{
			// Check if this child exists and add it to the queue
			if (childrenMask & (1 << i))
			{
				OctreeNodeTraversalEntry childEntry;
				childEntry.index = children[childrenStart + count];
				childEntry.position = GetChildPosition(entry.position, entry.size, i);
				childEntry.size = entry.size * 0.5f;

				nodesQueue.push(childEntry);

				count++;
			}
		}
    }
}

void PointCloudEngine::OctreeNode::GetVerticesAtLevel(std::queue<std::pair<OctreeNodeTraversalEntry, int>>& nodesQueue, std::vector<OctreeNodeVertex>& octreeVertices, const std::vector<UINT>& children, const OctreeNodeTraversalEntry& entry, const int& level) const
{
    if (level == 0)
    {
        octreeVertices.push_back(GetVertexFromTraversalEntry(entry));
    }
    else if (level > 0)
    {
		size_t count = 0;

        // Traverse the children
        for (int i = 0; i < 8; i++)
        {
			// Check if this child exists and add it to the queue
			if (childrenMask & (1 << i))
			{
				OctreeNodeTraversalEntry childEntry;
				childEntry.index = children[childrenStart + count];
				childEntry.position = GetChildPosition(entry.position, entry.size, i);
				childEntry.size = entry.size * 0.5f;

				nodesQueue.push(std::pair<OctreeNodeTraversalEntry, int>(childEntry, level - 1));

				count++;
			}
        }
    }
}

bool PointCloudEngine::OctreeNode::IsLeafNode() const
{
	return (childrenMask == 0);
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
	vertex.position = entry.position;
	vertex.size = entry.size;
	vertex.properties = properties;

	return vertex;
}
