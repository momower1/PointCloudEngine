#ifndef POINTCLOUDRENDERER_H
#define POINTCLOUDRENDERER_H

#pragma once
#include "PointCloudEngine.h"
#include "tinyply.h"

using namespace tinyply;

class PointCloudRenderer : public Component
{
public:
    static PointCloudRenderer* CreateShared(std::wstring plyfile);
    static void ReleaseAllSharedPointCloudRenderers();

    PointCloudRenderer(std::wstring plyfile);
    void Initialize(SceneObject *sceneObject);
    void Update(SceneObject *sceneObject);
    void Draw(SceneObject *sceneObject);
    void Release();

private:
    static std::vector<PointCloudRenderer*> sharedPointCloudRenderers;

    struct Vertex
    {
        Vector3 position;
        Vector3 normal;
        byte red, green, blue, alpha;
    };

    std::vector<Vertex> vertices;

    // Same constant buffer as in effect file
    struct ConstantBufferMatrices
    {
        Matrix World;
        Matrix View;
        Matrix Projection;
        Matrix WorldInverseTranspose;
    };

    // Vertex and index buffer
    ID3D11Buffer* vertexBuffer;		                // Holds vertex data

    ID3D11Buffer* constantBufferMatrices;		    // Stores data and sends it to the actual buffer in the effect file
};

#endif
