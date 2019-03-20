#ifndef OCTREE_H
#define OCTREE_H

#pragma once
#include "PointCloudEngine.h"

namespace PointCloudEngine
{
    class Octree
    {
    public:
        struct BoundingCube
        {
            // Bounding volume cube, size is the minimal distance from the center position to each face of the cube
            Vector3 position;
            Vector3 normal;
            float size;

            // Properties, TODO: add different normals and colors based on view direction
            byte red, green, blue, alpha;
        };

        struct Node
        {
            Node* parent = NULL;
            Node* children[8] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };

            BoundingCube boundingCube;
        };

        Octree (std::vector<PointCloudVertex> vertices);
        ~Octree();

        std::vector<BoundingCube> GetAllBoundingCubes();

    private:
        Node *root = NULL;

        Node* CreateNode(std::vector<PointCloudVertex> vertices);
        void SplitNode(Node *node, std::vector<PointCloudVertex> vertices);
        void DeleteNode(Node *node);
    };
}

#endif