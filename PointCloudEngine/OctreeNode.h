#ifndef OCTREENODE_H
#define OCTREENODE_H

#pragma once
#include "PointCloudEngine.h"

namespace PointCloudEngine
{
    class OctreeNode
    {
    public:
        OctreeNode (const std::vector<Vertex> &vertices, const Vector3 &center, OctreeNode *parent, const int &depth);
        ~OctreeNode();

        std::vector<OctreeNodeVertex> GetVertices(const Vector3 &localCameraPosition, const float &splatSize);
        std::vector<OctreeNodeVertex> GetVerticesAtLevel(const int &level);

    private:
        bool IsLeafNode();

        OctreeNode *parent = NULL;
        OctreeNode *children[8] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
        OctreeNodeVertex nodeVertex;
    };
}

#endif