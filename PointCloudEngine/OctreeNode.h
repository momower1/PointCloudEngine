#ifndef OCTREENODE_H
#define OCTREENODE_H

#pragma once
#include "PointCloudEngine.h"

namespace PointCloudEngine
{
    class OctreeNode
    {
    public:
        OctreeNode (std::queue<OctreeNodeCreationEntry> &nodeCreationQueue, std::vector<OctreeNode> &nodes, const OctreeNodeCreationEntry &entry);
        ~OctreeNode();

        std::vector<OctreeNodeVertex> GetVertices(const std::vector<OctreeNode> &nodes, const Vector3 &localCameraPosition, const float &splatSize) const;
        std::vector<OctreeNodeVertex> GetVerticesAtLevel(const std::vector<OctreeNode> &nodes, const int &level) const;
        bool IsLeafNode() const;

        // Negative index means that there is no child
        int children[8] = { -1, -1, -1, -1, -1, -1, -1, -1 };
        OctreeNodeVertex nodeVertex;
    };
}

#endif