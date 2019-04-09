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

    root = new OctreeNode(vertices, center, size, depth);
}

PointCloudEngine::Octree::~Octree()
{
    SafeDelete(root);
}

std::vector<OctreeNodeVertex> PointCloudEngine::Octree::GetVertices(const Vector3 &localCameraPosition, const float &splatSize)
{
    return root->GetVertices(localCameraPosition, splatSize);
}

std::vector<OctreeNodeVertex> PointCloudEngine::Octree::GetVerticesAtLevel(const int &level)
{
    return root->GetVerticesAtLevel(level);
}

void PointCloudEngine::Octree::GetRootPositionAndSize(Vector3 &outRootPosition, float &outSize)
{
    outRootPosition = root->nodeVertex.position;
    outSize = root->nodeVertex.size;
}