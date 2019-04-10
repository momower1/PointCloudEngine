#ifndef OCTREENODE_H
#define OCTREENODE_H

#pragma once
#include "PointCloudEngine.h"

namespace PointCloudEngine
{
    class OctreeNode
    {
    public:
        OctreeNode (std::vector<OctreeNode*> &nodes, int parentIndex, int childIndex, const int &depth, const std::vector<Vertex> &vertices, const Vector3 &center, const float &size);
        ~OctreeNode();

        std::vector<OctreeNodeVertex> GetVertices(const std::vector<OctreeNode*> &nodes, const Vector3 &localCameraPosition, const float &splatSize);
        std::vector<OctreeNodeVertex> GetVerticesAtLevel(const std::vector<OctreeNode*> &nodes, const int &level);
        bool IsLeafNode();

        // Negative index means that there is no child
        int children[8] = { -1, -1, -1, -1, -1, -1, -1, -1 };
        OctreeNodeVertex nodeVertex;
    };
}

#endif