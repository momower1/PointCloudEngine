#ifndef SPLATRENDERER_H
#define SPLATRENDERER_H

#pragma once
#include "PointCloudEngine.h"

namespace PointCloudEngine
{
    class SplatRenderer : public Component, public ISetSplatSize
    {
    public:
        SplatRenderer(std::wstring plyfile);
        void Initialize(SceneObject *sceneObject);
        void Update(SceneObject *sceneObject);
        void Draw(SceneObject *sceneObject);
        void Release();

        void SetSplatSize(const float &splatSize);

    private:
        // Same constant buffer as in effect file, keep packing rules in mind
        struct SplatRendererConstantBuffer
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

        std::vector<Vertex> vertices;
        SplatRendererConstantBuffer constantBufferData;

        // Vertex buffer
        ID3D11Buffer* vertexBuffer;		        // Holds vertex data
        ID3D11Buffer* constantBuffer;
    };
}
#endif
