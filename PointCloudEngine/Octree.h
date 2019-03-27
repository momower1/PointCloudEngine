#ifndef OCTREE_H
#define OCTREE_H

#pragma once
#include "PointCloudEngine.h"

namespace PointCloudEngine
{
    class Octree
    {
    public:
        static Vector3 viewDirections[6];

        Octree(const std::vector<Vertex> &vertices, const int &depth);
        ~Octree();

        std::vector<OctreeNodeVertex> GetVertices(const Vector3 &localCameraPosition, const float &splatSize);
        std::vector<OctreeNodeVertex> GetVerticesAtLevel(const int &level);

    private:
        OctreeNode *root = NULL;
    };
}

#endif