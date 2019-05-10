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

		void GetVertices(const std::vector<OctreeNode> &nodes, std::queue<OctreeNodeTraversalEntry>& nodesQueue, std::vector<OctreeNodeVertex>& octreeVertices, const OctreeNodeTraversalEntry& entry, const OctreeConstantBuffer& octreeConstantBufferData) const;
        bool IsLeafNode() const;

		// Stores either (1) the start index in the nodes array where the actual child indices are stored or (2) the leaf position factors
		// (1) The childrenMask from the properties determines which children corresponds to which index
		// (1) E.g. a childrenMask of 01011011 means that the array only stores the 2nd, 4th, 5th, 7th and 8th indices from the start right after each other
		// (2) When it is a leaf node with childrenMask=0 then this stores the average position of the vertices inside this bounding cube relative to the bounding cube size
		// (2) Each 8 bits store the distance factor from the smallest position of the bounding cube in respect to the size of the cube in each axis (x, y, z)
		UINT childrenStartOrLeafPositionFactors = 0;

		// Stores the childrenMask, weights, normals and colors
		OctreeNodeProperties properties;

	private:
		Vector3 GetChildPosition(const Vector3 &parentPosition, const float &parentSize, int childIndex) const;
		OctreeNodeVertex GetVertexFromTraversalEntry(const OctreeNodeTraversalEntry& entry) const;
    };
}

#endif