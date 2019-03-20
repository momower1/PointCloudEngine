#ifndef POINTCLOUDLODRENDERER_H
#define POINTCLOUDLODRENDERER_H

#pragma once
#include "PointCloudEngine.h"

namespace PointCloudEngine
{
    class PointCloudLODRenderer : public Component
    {
    public:
        static PointCloudLODRenderer* CreateShared(std::wstring plyfile);
        static void ReleaseAllSharedPointCloudLODRenderers();

        PointCloudLODRenderer(std::wstring plyfile);
        ~PointCloudLODRenderer();
        void Initialize(SceneObject *sceneObject);
        void Update(SceneObject *sceneObject);
        void Draw(SceneObject *sceneObject);
        void Release();

    private:
        static std::vector<PointCloudLODRenderer*> sharedPointCloudLODRenderers;

        // Same constant buffer as in effect file, keep packing rules in mind
        struct PointCloudLODConstantBuffer
        {
            float radius;
            float padding[3];
            Matrix World;
            Matrix View;
            Matrix Projection;
            Matrix WorldInverseTranspose;
        };

        Octree *octree = NULL;
        std::vector<PointCloudVertex> vertices;
        PointCloudLODConstantBuffer pointCloudLODConstantBufferData;

        // Vertex buffer
        ID3D11Buffer* vertexBuffer;		                    // Holds vertex data
        ID3D11Buffer* pointCloudLODConstantBuffer;		    // Stores data and sends it to the actual buffer in the effect file
    };
}
#endif
