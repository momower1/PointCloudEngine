#ifndef OCTREE_H
#define OCTREE_H

#pragma once
#include "PointCloudEngine.h"

namespace PointCloudEngine
{
    struct OctreeQueueEntry
    {
        // This is all the data needed to create an octree node
        int childIndex;
        std::vector<Vertex> vertices;
        Vector3 center;
        OctreeNode *parent;
    };

    class Octree
    {
    public:
        static std::queue<OctreeQueueEntry> octreeQueue;

        Octree(std::vector<Vertex> vertices);
        ~Octree();

        std::vector<OctreeNodeVertex> GetVertices(Vector3 localCameraPosition, float size);
        std::vector<OctreeNodeVertex> GetVerticesAtLevel(int level);

    private:
        OctreeNode *root = NULL;
    };
}

#endif