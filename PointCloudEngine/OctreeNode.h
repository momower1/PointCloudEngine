#ifndef OCTREENODE_H
#define OCTREENODE_H

#pragma once
#include "PointCloudEngine.h"

namespace PointCloudEngine
{
    struct OctreeNode
    {
    public:
        OctreeNode();
        OctreeNode (std::queue<OctreeNodeCreationEntry> &nodeCreationQueue, std::vector<OctreeNode> &nodes, const OctreeNodeCreationEntry &entry);

        void GetVertices(std::queue<UINT> &nodesQueue, std::vector<OctreeNodeVertex> &octreeVertices, const Vector3 &localCameraPosition, const float &splatSize) const;
        void GetVerticesAtLevel(std::queue<std::pair<UINT, int>> &nodesQueue, std::vector<OctreeNodeVertex> &octreeVertices, const int &level) const;
        bool IsLeafNode() const;

        // UINT_MAX index means that there is no child
        UINT children[8] =
        {
            UINT_MAX,
            UINT_MAX,
            UINT_MAX,
            UINT_MAX,
            UINT_MAX,
            UINT_MAX,
            UINT_MAX,
            UINT_MAX
        };

        OctreeNodeVertex nodeVertex;
    };
}

#endif