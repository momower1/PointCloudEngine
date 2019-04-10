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

    new OctreeNode(nodes, -1, -1, depth, vertices, center, size);
}

PointCloudEngine::Octree::~Octree()
{
    // Delete the whole array of nodes
    for (auto it = nodes.begin(); it != nodes.end(); it++)
    {
        SafeDelete(*it);
    }

    nodes.clear();
}

std::vector<OctreeNodeVertex> PointCloudEngine::Octree::GetVertices(const Vector3 &localCameraPosition, const float &splatSize)
{
    return nodes[0]->GetVertices(nodes, localCameraPosition, splatSize);
}

std::vector<OctreeNodeVertex> PointCloudEngine::Octree::GetVerticesAtLevel(const int &level)
{
    return nodes[0]->GetVerticesAtLevel(nodes, level);
}

void PointCloudEngine::Octree::GetRootPositionAndSize(Vector3 &outRootPosition, float &outSize)
{
    outRootPosition = nodes[0]->nodeVertex.position;
    outSize = nodes[0]->nodeVertex.size;
}