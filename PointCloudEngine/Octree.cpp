#include "Octree.h"

PointCloudEngine::Octree::Octree(std::vector<PointCloudVertex> vertices)
{
    root = CreateNode(vertices);
}

PointCloudEngine::Octree::~Octree()
{
    DeleteNode(root);
}

Octree::Node* PointCloudEngine::Octree::CreateNode(std::vector<PointCloudVertex> vertices)
{
    size_t size = vertices.size();

    if (size == 0)
    {
        ErrorMessage(L"Cannot create Octree node from empty vertices!", L"CreateNode", __FILEW__, __LINE__);
        return NULL;
    }

    Node* node = new Node();

    // Initialize average color
    double averageRed = 0;
    double averageGreen = 0;
    double averageBlue = 0;
    double averageAlpha = 0;
    double factor = 255.0f * size;

    // Initialize min/max bounds
    node->vertex.min = node->vertex.max = vertices.front().position;

    for (auto it = vertices.begin(); it != vertices.end(); it++)
    {
        PointCloudVertex v = *it;

        node->vertex.min = Vector3::Min(node->vertex.min, v.position);
        node->vertex.max = Vector3::Max(node->vertex.max, v.position);

        averageRed = min(1, averageRed + v.red / factor);
        averageGreen = min(1, averageGreen + v.green / factor);
        averageBlue = min(1, averageBlue + v.blue / factor);
        averageAlpha = min(1, averageAlpha + v.alpha / factor);
    }

    // Save average color
    node->vertex.red = averageRed * 255.0f;
    node->vertex.green = averageGreen * 255.0f;
    node->vertex.blue = averageBlue * 255.0f;
    node->vertex.alpha = averageAlpha * 255.0f;

    return node;
}

void PointCloudEngine::Octree::SplitNode(Node * node, std::vector<PointCloudVertex> vertices)
{
}

void PointCloudEngine::Octree::DeleteNode(Node *node)
{
    if (node != NULL)
    {
        // Delete leaf nodes first
        for (int i = 0; i < 8; i++)
        {
            if (node->children[i] != NULL)
            {
                DeleteNode(node->children[i]);
            }
        }

        SafeDelete(node);
    }
}
