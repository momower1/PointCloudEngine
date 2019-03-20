#include "Octree.h"

PointCloudEngine::Octree::Octree(std::vector<PointCloudVertex> vertices)
{
    root = CreateNode(vertices);
}

PointCloudEngine::Octree::~Octree()
{
    DeleteNode(root);
}

std::vector<Octree::BoundingCube> PointCloudEngine::Octree::GetAllBoundingCubes()
{
    std::vector<BoundingCube> boundingCubes;

    boundingCubes.push_back(root->boundingCube);

    // TODO: Actually traverse the whole octree and only add bounding boxes at certain conditions

    return boundingCubes;
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

    // Initialize min/max for the cube bounds
    Vector3 minPosition = vertices.front().position;
    Vector3 maxPosition = minPosition;

    // TODO: Normal

    for (auto it = vertices.begin(); it != vertices.end(); it++)
    {
        PointCloudVertex v = *it;

        minPosition = Vector3::Min(minPosition, v.position);
        maxPosition = Vector3::Max(maxPosition, v.position);

        averageRed += v.red / factor;
        averageGreen += v.green / factor;
        averageBlue += v.blue / factor;
        averageAlpha += v.alpha / factor;
    }

    // Save the bounding cube properties
    node->boundingCube.position = minPosition + 0.5f * (maxPosition - minPosition);
    node->boundingCube.size = max(max(maxPosition.x - minPosition.x, maxPosition.y - minPosition.y), maxPosition.z - minPosition.z);

    // Save average color
    node->boundingCube.red = round(averageRed * 255);
    node->boundingCube.green = round(averageGreen * 255);
    node->boundingCube.blue = round(averageBlue * 255);
    node->boundingCube.alpha = round(averageAlpha * 255);

    return node;
}

void PointCloudEngine::Octree::SplitNode(Node* node, std::vector<PointCloudVertex> vertices)
{
    // TODO
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
