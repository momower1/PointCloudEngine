#include "Octree.h"

PointCloudEngine::Octree::Octree(std::vector<Vertex> vertices)
{
    root = new OctreeNode(vertices, Vector3::Zero, NULL);
}

PointCloudEngine::Octree::~Octree()
{
    SafeDelete(root);
}

std::vector<OctreeNodeVertex> PointCloudEngine::Octree::GetVertices(Vector3 localCameraPosition, float size)
{
    return root->GetVertices(localCameraPosition, size);
}

std::vector<OctreeNodeVertex> PointCloudEngine::Octree::GetVerticesAtLevel(int level)
{
    return root->GetVerticesAtLevel(level);
}
