#ifndef OCTREERENDERER_H
#define OCTREERENDERER_H

#pragma once
#include "PointCloudEngine.h"

namespace PointCloudEngine
{
    class OctreeRenderer : public Component, public IRenderer
    {
    public:
        OctreeRenderer(const std::vector<Vertex> &vertices);
        void Initialize(SceneObject *sceneObject);
        void Update(SceneObject *sceneObject);
        void Draw(SceneObject *sceneObject);
        void Release();

        void SetSplatSize(const float& splatSize);

    private:
        // Same constant buffer as in effect file, keep packing rules in mind
        struct OctreeRendererConstantBuffer
        {
            Matrix World;
            Matrix View;
            Matrix Projection;
            Matrix WorldInverseTranspose;
            Vector3 cameraPosition;
            float fovAngleY;
            float splatSize;
            float padding[3];
        };

        int level = -1;
        int viewMode = 0;

        Octree *octree = NULL;
        SceneObject *text = NULL;
        TextRenderer *textRenderer = NULL;
        std::vector<OctreeNodeVertex> octreeVertices;
        
        OctreeRendererConstantBuffer constantBufferData;

        // Vertex buffer
        UINT vertexBufferSize = 0;
        ID3D11Buffer* vertexBuffer;		        // Holds vertex data
        ID3D11Buffer* constantBuffer;		    // Stores data and sends it to the actual buffer in the effect file
    };
}
#endif
