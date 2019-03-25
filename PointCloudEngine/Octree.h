#ifndef OCTREE_H
#define OCTREE_H

#pragma once
#include "PointCloudEngine.h"

namespace PointCloudEngine
{
    struct OctreeCreationQueueEntry
    {
        // This is all the data needed to create an octree node
        int childIndex;
        std::vector<PointCloudVertex> vertices;
        Vector3 center;
        Octree *parent;
    };

    class Octree
    {
    public:
        // Avoids recursion by using a queue (recursion can cause a stack overflow with large files)
        static Octree* Create(std::vector<PointCloudVertex> vertices);

        Octree (std::vector<PointCloudVertex> vertices, Vector3 center, Octree *parent, int childIndex);
        ~Octree();

        std::vector<OctreeVertex> GetOctreeVertices(Vector3 localCameraPosition, float size);
        std::vector<OctreeVertex> GetOctreeVerticesAtLevel(int level);

    private:
        bool IsLeafNode();

        static std::queue<OctreeCreationQueueEntry> octreeCreationQueue;

        // Reducing this value heavily decreases the octree node count and therefore also the memory requirements
        // TODO: Should be set individually for each ply model
        const float octreeVertexMinSize = 0.0001f;

        Octree *parent = NULL;
        Octree *children[8] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
        OctreeVertex octreeVertex;
    };
}

#endif