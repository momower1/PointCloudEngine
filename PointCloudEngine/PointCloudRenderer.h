#ifndef POINTCLOUDRENDERER_H
#define POINTCLOUDRENDERER_H

#pragma once
#include "PointCloudEngine.h"

namespace PointCloudEngine
{
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

        // Same constant buffer as in effect file, keep packing rules in mind
        struct PointCloudConstantBuffer
        {
            Matrix World;
            Matrix View;
            Matrix Projection;
            Matrix WorldInverseTranspose;
            Vector3 cameraPosition;
            float radius;
        };

        std::vector<PointCloudVertex> vertices;
        PointCloudConstantBuffer pointCloudConstantBufferData;

        // Vertex buffer
        ID3D11Buffer* vertexBuffer;		                // Holds vertex data
        ID3D11Buffer* pointCloudConstantBuffer;		    // Stores data and sends it to the actual buffer in the effect file
    };
}
#endif
