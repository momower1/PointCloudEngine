#ifndef POINTCLOUDLODRENDERER_H
#define POINTCLOUDLODRENDERER_H

#pragma once
#include "PointCloudEngine.h"

namespace PointCloudEngine
{
    class PointCloudLODRenderer : public Component
    {
    public:
        PointCloudLODRenderer(std::wstring plyfile);
        ~PointCloudLODRenderer();
        void Initialize(SceneObject *sceneObject);
        void Update(SceneObject *sceneObject);
        void Draw(SceneObject *sceneObject);
        void Release();

    private:
        // Same constant buffer as in effect file, keep packing rules in mind
        struct PointCloudLODConstantBuffer
        {
            Matrix World;
            Matrix View;
            Matrix Projection;
            Matrix WorldInverseTranspose;
        };

        int level = -1;
        float radius = 0.02f;
        Octree *octree = NULL;
        SceneObject *text = NULL;
        TextRenderer *textRenderer = NULL;
        std::vector<OctreeNodeVertex> octreeVertices;
        
        PointCloudLODConstantBuffer pointCloudLODConstantBufferData;

        // Vertex buffer
        UINT vertexBufferSize = 0;
        ID3D11Buffer* vertexBuffer;		                    // Holds vertex data
        ID3D11Buffer* pointCloudLODConstantBuffer;		    // Stores data and sends it to the actual buffer in the effect file
    };
}
#endif
