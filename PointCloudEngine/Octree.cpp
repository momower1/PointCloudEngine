#include "Octree.h"

PointCloudEngine::Octree::Octree(std::vector<PointCloudVertex> vertices)
{
    size_t size = vertices.size();

    if (size == 0)
    {
        ErrorMessage(L"Cannot create Octree node from empty vertices!", L"CreateNode", __FILEW__, __LINE__);
    }

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
    boundingCube.position = minPosition + 0.5f * (maxPosition - minPosition);
    boundingCube.size = max(max(maxPosition.x - minPosition.x, maxPosition.y - minPosition.y), maxPosition.z - minPosition.z);

    // Save average color
    boundingCube.red = round(averageRed * 255);
    boundingCube.green = round(averageGreen * 255);
    boundingCube.blue = round(averageBlue * 255);
    boundingCube.alpha = round(averageAlpha * 255);

    // TODO: Split and create children
    std::vector<PointCloudVertex> childVertices[8];

    for (auto it = vertices.begin(); it != vertices.end(); it++)
    {
        PointCloudVertex v = *it;

        if (v.position.x > boundingCube.position.x && v.position.y > boundingCube.position.y && v.position.z > boundingCube.position.z)
        {
            childVertices[0].push_back(v);
        }
        else if (v.position.x <= boundingCube.position.x && v.position.y > boundingCube.position.y && v.position.z > boundingCube.position.z)
        {
            childVertices[1].push_back(v);
        }
        else if (v.position.x > boundingCube.position.x && v.position.y <= boundingCube.position.y && v.position.z > boundingCube.position.z)
        {
            childVertices[2].push_back(v);
        }
        else if (v.position.x > boundingCube.position.x && v.position.y > boundingCube.position.y && v.position.z <= boundingCube.position.z)
        {
            childVertices[3].push_back(v);
        }
        else if (v.position.x <= boundingCube.position.x && v.position.y <= boundingCube.position.y && v.position.z > boundingCube.position.z)
        {
            childVertices[4].push_back(v);
        }
        else if (v.position.x > boundingCube.position.x && v.position.y <= boundingCube.position.y && v.position.z <= boundingCube.position.z)
        {
            childVertices[5].push_back(v);
        }
        else if (v.position.x <= boundingCube.position.x && v.position.y > boundingCube.position.y && v.position.z <= boundingCube.position.z)
        {
            childVertices[6].push_back(v);
        }
        else if (v.position.x <= boundingCube.position.x && v.position.y <= boundingCube.position.y && v.position.z <= boundingCube.position.z)
        {
            childVertices[7].push_back(v);
        }
    }

    for (int i = 0; i < 8; i++)
    {
        // TODO: This ending condition has to be different because there should be leaf nodes with the actual vertices
        if (childVertices[i].size() > 1)
        {
            children[i] = new Octree(childVertices[i]);
            children[i]->parent = this;
        }
    }
}

PointCloudEngine::Octree::~Octree()
{
    // Delete children
    for (int i = 0; i < 8; i++)
    {
        SafeDelete(children[i]);
    }
}

std::vector<Octree::BoundingCube> PointCloudEngine::Octree::GetBoundingCubesAtLevel(int level)
{
    std::vector<BoundingCube> boundingCubes;

    if (level == 0)
    {
        boundingCubes.push_back(boundingCube);
    }
    else if (level > 0)
    {
        // Traverse the whole octree and add child bounding cubes
        for (int i = 0; i < 8; i++)
        {
            if (children[i] != NULL)
            {
                std::vector<BoundingCube> childBoundingCubes = children[i]->GetBoundingCubesAtLevel(level - 1);
                boundingCubes.insert(boundingCubes.end(), childBoundingCubes.begin(), childBoundingCubes.end());
            }
        }
    }

    return boundingCubes;
}
