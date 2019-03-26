#ifndef OCTREE_H
#define OCTREE_H

#pragma once
#include "PointCloudEngine.h"

namespace PointCloudEngine
{
    class Octree
    {
    public:
        Octree(std::vector<Vertex> vertices);
        ~Octree();

        std::vector<OctreeNodeVertex> GetVertices(Vector3 localCameraPosition, float size);
        std::vector<OctreeNodeVertex> GetVerticesAtLevel(int level);

    private:
        OctreeNode *root = NULL;
    };
}

#endif