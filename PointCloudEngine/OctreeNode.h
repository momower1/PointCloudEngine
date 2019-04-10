#ifndef OCTREENODE_H
#define OCTREENODE_H

#pragma once
#include "PointCloudEngine.h"

namespace PointCloudEngine
{
    class OctreeNode
    {
    public:
        OctreeNode();
        OctreeNode (std::queue<OctreeNodeCreationEntry> &nodeCreationQueue, std::vector<OctreeNode> &nodes, const OctreeNodeCreationEntry &entry);
        ~OctreeNode();

        void GetVertices(std::queue<int> &nodesQueue, std::vector<OctreeNodeVertex> &octreeVertices, const Vector3 &localCameraPosition, const float &splatSize) const;
        void GetVerticesAtLevel(std::queue<std::pair<int, int>> &nodesQueue, std::vector<OctreeNodeVertex> &octreeVertices, const int &level) const;
        bool IsLeafNode() const;

        // Negative index means that there is no child
        int children[8] = { -1, -1, -1, -1, -1, -1, -1, -1 };
        OctreeNodeVertex nodeVertex;
    };
}

#endif