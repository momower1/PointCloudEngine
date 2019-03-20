#ifndef OCTREE_H
#define OCTREE_H

#pragma once
#include "PointCloudEngine.h"

namespace PointCloudEngine
{
    class Octree
    {
    public:
        struct Vertex
        {
            // Bounding volume
            Vector3 min;
            Vector3 max;

            // Properties, TODO: add different normals and colors based on view direction
            byte red, green, blue, alpha;
        };

        struct Node
        {
            Node* parent = NULL;
            Node* children[8] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };

            Vertex vertex;
        };

        Octree (std::vector<PointCloudVertex> vertices);
        ~Octree();

    private:
        Node *root = NULL;

        Node* CreateNode(std::vector<PointCloudVertex> vertices);
        void SplitNode(Node *node, std::vector<PointCloudVertex> vertices);
        void DeleteNode(Node *node);
    };
}

#endif