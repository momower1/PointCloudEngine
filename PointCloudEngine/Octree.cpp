#include "Octree.h"

PointCloudEngine::Octree::Octree(const std::vector<Vertex> &vertices, const int &depth)
{
    // Calculate center and size of the root node
    Vector3 minPosition = vertices.front().position;
    Vector3 maxPosition = minPosition;

    for (auto it = vertices.begin(); it != vertices.end(); it++)
    {
        Vertex v = *it;

        minPosition = Vector3::Min(minPosition, v.position);
        maxPosition = Vector3::Max(maxPosition, v.position);
    }

    Vector3 diagonal = maxPosition - minPosition;
    Vector3 center = minPosition + 0.5f * (diagonal);
    float size = max(max(diagonal.x, diagonal.y), diagonal.z);

    // Stores the nodes that should be created for each octree level
    std::queue<OctreeNodeCreationEntry> nodeCreationQueue;

    OctreeNodeCreationEntry rootEntry;
    rootEntry.nodeIndex = -1;
    rootEntry.parentIndex = -1;
    rootEntry.parentChildIndex = -1;
    rootEntry.vertices = vertices;
    rootEntry.center = center;
    rootEntry.size = size;
    rootEntry.depth = depth;

    nodeCreationQueue.push(rootEntry);

    while (!nodeCreationQueue.empty())
    {
        // Remove the first entry from the queue
        OctreeNodeCreationEntry first = nodeCreationQueue.front();
        nodeCreationQueue.pop();

        // Assign the index at which this node will be stored
        first.nodeIndex = nodes.size();

        // Create the nodes and fill the queue
        nodes.push_back(OctreeNode(nodeCreationQueue, nodes, first));
    }
}

PointCloudEngine::Octree::~Octree()
{
    nodes.clear();
}

std::vector<OctreeNodeVertex> PointCloudEngine::Octree::GetVertices(const Vector3 &localCameraPosition, const float &splatSize) const
{
    return nodes[0].GetVertices(nodes, localCameraPosition, splatSize);
}

std::vector<OctreeNodeVertex> PointCloudEngine::Octree::GetVerticesAtLevel(const int &level) const
{
    return nodes[0].GetVerticesAtLevel(nodes, level);
}

void PointCloudEngine::Octree::GetRootPositionAndSize(Vector3 &outRootPosition, float &outSize) const
{
    outRootPosition = nodes[0].nodeVertex.position;
    outSize = nodes[0].nodeVertex.size;
}