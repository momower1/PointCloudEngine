#include "Octree.h"

PointCloudEngine::Octree::Octree(const std::vector<Vertex> &vertices, const int &depth)
{
    root = new OctreeNode(vertices, Vector3::Zero, NULL, depth);
}

PointCloudEngine::Octree::~Octree()
{
    SafeDelete(root);
}

std::vector<OctreeNodeVertex> PointCloudEngine::Octree::GetVertices(const Vector3 &localCameraPosition, const float &size)
{
    return root->GetVertices(localCameraPosition, size);
}

std::vector<OctreeNodeVertex> PointCloudEngine::Octree::GetVerticesAtLevel(const int &level)
{
    return root->GetVerticesAtLevel(level);
}
