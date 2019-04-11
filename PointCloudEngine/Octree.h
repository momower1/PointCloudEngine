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

        std::vector<OctreeNodeVertex> GetVertices(const Vector3 &localCameraPosition, const float &splatSize) const;
        std::vector<OctreeNodeVertex> GetVerticesAtLevel(const int &level) const;
        void GetRootPositionAndSize(Vector3 &outRootPosition, float &outSize) const;

    private:
        bool LoadFromOctreeFile();
        void SaveToOctreeFile();

        // Stores the hole octree, the root is the first element
        std::vector<OctreeNode> nodes;
        std::wstring octreeFilepath;
    };
}

#endif