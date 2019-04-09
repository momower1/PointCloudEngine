#ifndef OCTREE_H
#define OCTREE_H

#pragma once
#include "PointCloudEngine.h"

namespace PointCloudEngine
{
    class Octree
    {
    public:
        Octree(const std::vector<Vertex> &vertices, const int &depth);
        ~Octree();

        std::vector<OctreeNodeVertex> GetVertices(const Vector3 &localCameraPosition, const float &splatSize);
        std::vector<OctreeNodeVertex> GetVerticesAtLevel(const int &level);
        void GetRootPositionAndSize(Vector3 &outRootPosition, float &outSize);

    private:
        OctreeNode *root = NULL;
    };
}

#endif