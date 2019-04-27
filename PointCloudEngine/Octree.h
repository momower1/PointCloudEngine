#ifndef OCTREE_H
#define OCTREE_H

#pragma once
#include "PointCloudEngine.h"

namespace PointCloudEngine
{
    class Octree
    {
    public:
        Octree(const std::wstring &plyfile);

        std::vector<OctreeNodeVertex> GetVertices(const Vector3 &localCameraPosition, const float &splatSize, const int &level) const;
        bool LoadFromOctreeFile();
        void SaveToOctreeFile();

        // Stores the hole octree, the root is the first element then all the children of the root node follow and so on
        std::vector<OctreeNode> nodes;
		Vector3 rootPosition;
		float rootSize = 0;

	private:
		std::wstring octreeFilepath;
    };
}

#endif