#ifndef OCTREE_H
#define OCTREE_H

#pragma once
#include "PointCloudEngine.h"

namespace PointCloudEngine
{
    struct OctreeVertex
    {
        // Bounding volume
        Vector3 min;
        Vector3 max;

        // Properties, TODO: add different normals and colors based on view direction
        Color color;
    };

    struct OctreeNode
    {
        OctreeNode* parent;
        OctreeNode* children[8];

        OctreeVertex vertex;
    };

    class Octree
    {
    public:
        Octree (std::vector<PointCloudVertex> vertices);

    private:
        OctreeNode *root;
    };
}

#endif