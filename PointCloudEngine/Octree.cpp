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
    octreeVertex.position = minPosition + 0.5f * (maxPosition - minPosition);
    octreeVertex.size = max(max(maxPosition.x - minPosition.x, maxPosition.y - minPosition.y), maxPosition.z - minPosition.z);

    // Save average color
    octreeVertex.red = round(averageRed * 255);
    octreeVertex.green = round(averageGreen * 255);
    octreeVertex.blue = round(averageBlue * 255);
    octreeVertex.alpha = round(averageAlpha * 255);

    // Split and create children
    std::vector<PointCloudVertex> childVertices[8];

    // TODO: Optimize by using smarter if-else without &&
    for (auto it = vertices.begin(); it != vertices.end(); it++)
    {
        PointCloudVertex v = *it;

        if (v.position.x > octreeVertex.position.x && v.position.y > octreeVertex.position.y && v.position.z > octreeVertex.position.z)
        {
            childVertices[0].push_back(v);
        }
        else if (v.position.x <= octreeVertex.position.x && v.position.y > octreeVertex.position.y && v.position.z > octreeVertex.position.z)
        {
            childVertices[1].push_back(v);
        }
        else if (v.position.x > octreeVertex.position.x && v.position.y <= octreeVertex.position.y && v.position.z > octreeVertex.position.z)
        {
            childVertices[2].push_back(v);
        }
        else if (v.position.x > octreeVertex.position.x && v.position.y > octreeVertex.position.y && v.position.z <= octreeVertex.position.z)
        {
            childVertices[3].push_back(v);
        }
        else if (v.position.x <= octreeVertex.position.x && v.position.y <= octreeVertex.position.y && v.position.z > octreeVertex.position.z)
        {
            childVertices[4].push_back(v);
        }
        else if (v.position.x > octreeVertex.position.x && v.position.y <= octreeVertex.position.y && v.position.z <= octreeVertex.position.z)
        {
            childVertices[5].push_back(v);
        }
        else if (v.position.x <= octreeVertex.position.x && v.position.y > octreeVertex.position.y && v.position.z <= octreeVertex.position.z)
        {
            childVertices[6].push_back(v);
        }
        else if (v.position.x <= octreeVertex.position.x && v.position.y <= octreeVertex.position.y && v.position.z <= octreeVertex.position.z)
        {
            childVertices[7].push_back(v);
        }
    }

    // Don't subdivide further if the size is below the minimum size
    if (octreeVertex.size > octreeVertexMinSize)
    {
        for (int i = 0; i < 8; i++)
        {
            if (childVertices[i].size() > 0)
            {
                children[i] = new Octree(childVertices[i]);
                children[i]->parent = this;
            }
        }
    }
    else
    {
        octreeVertex.size = octreeVertexMinSize;
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

std::vector<OctreeVertex> PointCloudEngine::Octree::GetOctreeVertices(Vector3 localCameraPosition, float size)
{
    // TODO: View frustum culling, Visibility culling (normals)
    // Only return a vertex if its projected size is smaller than the passed size or it is a leaf node
    std::vector<OctreeVertex> octreeVertices;
    float distanceToCamera = Vector3::Distance(localCameraPosition, octreeVertex.position);

    // Scale the size by the fov and camera distance
    float worldSize = size * (2.0f * tan(fovAngleY / 2.0f)) * distanceToCamera;

    if ((octreeVertex.size < worldSize) || IsLeafNode())
    {
        octreeVertices.push_back(octreeVertex);
    }
    else
    {
        // Traverse the whole octree and add child vertices
        for (int i = 0; i < 8; i++)
        {
            if (children[i] != NULL)
            {
                std::vector<OctreeVertex> childOctreeVertices = children[i]->GetOctreeVertices(localCameraPosition, size);
                octreeVertices.insert(octreeVertices.end(), childOctreeVertices.begin(), childOctreeVertices.end());
            }
        }
    }

    return octreeVertices;
}

std::vector<OctreeVertex> PointCloudEngine::Octree::GetOctreeVerticesAtLevel(int level)
{
    std::vector<OctreeVertex> octreeVertices;

    if (level == 0)
    {
        octreeVertices.push_back(octreeVertex);
    }
    else if (level > 0)
    {
        // Traverse the whole octree and add child vertices
        for (int i = 0; i < 8; i++)
        {
            if (children[i] != NULL)
            {
                std::vector<OctreeVertex> childOctreeVertices = children[i]->GetOctreeVerticesAtLevel(level - 1);
                octreeVertices.insert(octreeVertices.end(), childOctreeVertices.begin(), childOctreeVertices.end());
            }
        }
    }

    return octreeVertices;
}

bool PointCloudEngine::Octree::IsLeafNode()
{
    for (int i = 0; i < 8; i++)
    {
        if (children[i] != NULL)
        {
            return false;
        }
    }

    return true;
}
