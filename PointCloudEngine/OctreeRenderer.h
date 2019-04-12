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
        void DrawOctree(SceneObject *sceneObject);
        void DrawOctreeCompute(SceneObject *sceneObject);

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
            float overlapFactor;
            float padding[2];
        };

        int level = -1;
        int viewMode = 0;
        bool useComputeShader = false;

        Octree *octree = NULL;
        SceneObject *text = NULL;
        TextRenderer *textRenderer = NULL;
        std::vector<OctreeNodeVertex> octreeVertices;
        
        OctreeRendererConstantBuffer constantBufferData;

        // Vertex buffer
        UINT vertexBufferSize = 0;
        ID3D11Buffer* vertexBuffer = NULL;		        // Holds vertex data
        ID3D11Buffer* constantBuffer = NULL;		    // Stores data and sends it to the actual buffer in the effect file

        // Compute shader
        ID3D11ComputeShader *computeShader = NULL;
        ID3D11Buffer *computeShaderBuffer = NULL;
    };
}
#endif
