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

        void GetVertices(std::queue<OctreeNodeTraversalEntry> &nodesQueue, std::vector<OctreeNodeVertex> &octreeVertices, const OctreeNodeTraversalEntry &entry, const Vector3 &localCameraPosition, const float &splatSize) const;
        void GetVerticesAtLevel(std::queue<std::pair<OctreeNodeTraversalEntry, int>> &nodesQueue, std::vector<OctreeNodeVertex> &octreeVertices, const OctreeNodeTraversalEntry &entry, const int &level) const;
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

		OctreeNodeProperties properties;

	private:
		Vector3 GetChildPosition(const Vector3 &parentPosition, const float &parentSize, int childIndex) const;
		OctreeNodeVertex GetVertexFromTraversalEntry(const OctreeNodeTraversalEntry& entry) const;
    };
}

#endif