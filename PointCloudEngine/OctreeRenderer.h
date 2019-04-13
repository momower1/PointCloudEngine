#ifndef OCTREERENDERER_H
#define OCTREERENDERER_H

#pragma once
#include "PointCloudEngine.h"

namespace PointCloudEngine
{
    class OctreeRenderer : public Component, public IRenderer
    {
    public:
        OctreeRenderer(const std::wstring &plyfile);
        void Initialize(SceneObject *sceneObject);
        void Update(SceneObject *sceneObject);
        void Draw(SceneObject *sceneObject);
        void Release();

        void SetSplatSize(const float& splatSize);
        void GetBoundingCubePositionAndSize(Vector3 &outPosition, float &outSize);

    private:
        void DrawOctree(SceneObject *sceneObject, const Vector3 &localCameraPosition);
        void DrawOctreeCompute(SceneObject *sceneObject, const Vector3 &localCameraPosition);

        // Same constant buffer as in hlsl file, keep packing rules in mind
        struct OctreeRendererConstantBuffer
        {
            Matrix World;
            Matrix View;
            Matrix Projection;
            Matrix WorldInverseTranspose;
            Vector3 cameraPosition;
            float fovAngleY;
            float splatSize;
            float overlapFactor;
            float padding[2];
        };

        struct ComputeShaderConstantBuffer
        {
            Vector3 localCameraPosition;
            float splatSize;
        };

        int level = -1;
        int viewMode = 0;
        bool useComputeShader = true;
        int vertexBufferSize = 0;

        Octree *octree = NULL;
        SceneObject *text = NULL;
        TextRenderer *textRenderer = NULL;

        // Renderer buffer
        ID3D11Buffer* octreeRendererConstantBuffer = NULL;
        OctreeRendererConstantBuffer octreeRendererConstantBufferData;

        // Compute shader
        ID3D11Buffer *nodesBuffer = NULL;
        ID3D11ShaderResourceView *nodesBufferSRV = NULL;
        ID3D11Buffer *computeShaderConstantBuffer = NULL;
        ComputeShaderConstantBuffer computeShaderConstantBufferData;
    };
}
#endif
