#include "Octree.h"

// Avoids recursion by using a queue (recursion can cause a stack overflow with large files)
std::queue<OctreeQueueEntry> Octree::octreeQueue;

PointCloudEngine::Octree::Octree(std::vector<Vertex> vertices)
{
    // This is the root node
    root = new OctreeNode(vertices, Vector3::Zero, NULL, 0);

    while (!octreeQueue.empty())
    {
        // Remove the first element from the queue and create the corresponding octree
        // Parents are always closer to the front of the queue than their children
        OctreeQueueEntry entry = octreeQueue.front();
        octreeQueue.pop();

        // This is quite risky but we count on the parent to delete this octree child later
        new OctreeNode(entry.vertices, entry.center, entry.parent, entry.childIndex);
    }
}

PointCloudEngine::Octree::~Octree()
{
    SafeDelete(root);
}

std::vector<OctreeNodeVertex> PointCloudEngine::Octree::GetVertices(Vector3 localCameraPosition, float size)
{
    return root->GetOctreeVertices(localCameraPosition, size);
}

std::vector<OctreeNodeVertex> PointCloudEngine::Octree::GetVerticesAtLevel(int level)
{
    return root->GetOctreeVerticesAtLevel(level);
}
