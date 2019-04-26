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
        OctreeNode (std::queue<OctreeNodeCreationEntry> &nodeCreationQueue, std::vector<OctreeNode> &nodes, std::vector<UINT> &children, const OctreeNodeCreationEntry &entry);

        void GetVertices(std::queue<OctreeNodeTraversalEntry> &nodesQueue, std::vector<OctreeNodeVertex> &octreeVertices, const std::vector<UINT> &children, const OctreeNodeTraversalEntry &entry, const Vector3 &localCameraPosition, const float &splatSize) const;
        void GetVerticesAtLevel(std::queue<std::pair<OctreeNodeTraversalEntry, int>> &nodesQueue, std::vector<OctreeNodeVertex> &octreeVertices, const std::vector<UINT> &children, const OctreeNodeTraversalEntry &entry, const int &level) const;
        bool IsLeafNode() const;

		// Stores the start index in the children array where the actual child indices are stored
		// The childrenMask from the properties determines which children corresponds to which index
		// E.g. a childrenMask of 01011011 means that the array only stores the 2nd, 4th, 5th, 7th and 8th indices from the start right after each other
		UINT childrenStart = 0;

		// Stores the childrenMask, weights, normals and colors
		OctreeNodeProperties properties;

	private:
		Vector3 GetChildPosition(const Vector3 &parentPosition, const float &parentSize, int childIndex) const;
		OctreeNodeVertex GetVertexFromTraversalEntry(const OctreeNodeTraversalEntry& entry) const;
    };
}

#endif