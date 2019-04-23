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

		// Mask where the lowest 8 bit store one bit for each of the children: 1 when it exists, 0 when it doesn't
		// TODO: This could easily be stored in one byte but the shader can only parse 32bit values -> maybe move into weights?
		UINT childrenMask = 0;

		// Stores the start index in the children array where the actual child indices are stored
		// The mask determines which children corresponds to which index e.g. 01011011 would only store the 2nd, 4th, 5th, 7th and 8th indices in the array right after each other
		// E.g. when there are 5 children then this is the index of the first child, then the 4 following array entries are the other indices
		UINT childrenStart = 0;

		OctreeNodeProperties properties;

	private:
		Vector3 GetChildPosition(const Vector3 &parentPosition, const float &parentSize, int childIndex) const;
		OctreeNodeVertex GetVertexFromTraversalEntry(const OctreeNodeTraversalEntry& entry) const;
    };
}

#endif