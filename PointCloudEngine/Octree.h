#ifndef OCTREE_H
#define OCTREE_H

#pragma once
#include "PointCloudEngine.h"

namespace PointCloudEngine
{
    class Octree
    {
    public:
        Octree (std::vector<PointCloudVertex> vertices, Vector3 center);
        ~Octree();

        std::vector<OctreeVertex> GetOctreeVertices(Vector3 localCameraPosition, float size);
        std::vector<OctreeVertex> GetOctreeVerticesAtLevel(int level);

    private:
        bool IsLeafNode();

        const float octreeVertexMinSize = FLT_EPSILON;

        Octree *parent = NULL;
        Octree *children[8] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
        OctreeVertex octreeVertex;
    };
}

#endif