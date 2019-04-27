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
        UINT GetStructureCount(ID3D11UnorderedAccessView *UAV);

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
			float samplingRate;
            float overlapFactor;
            float padding;
        };

        struct ComputeShaderConstantBuffer
        {
            Vector3 localCameraPosition;
            float fovAngleY;
            float splatSize;
			int level;
            UINT inputCount;
            float padding;
        };

        int level = -1;
        int viewMode = 0;
        int vertexBufferCount = 0;
        bool useComputeShader = false;

        Octree *octree = NULL;
        SceneObject *text = NULL;
        TextRenderer *textRenderer = NULL;

        // Renderer buffer
        ID3D11Buffer* octreeRendererConstantBuffer = NULL;
        OctreeRendererConstantBuffer octreeRendererConstantBufferData;

        // Compute shader
        UINT maxVertexBufferCount = 15000000;
        ID3D11Buffer *nodesBuffer = NULL;
		ID3D11Buffer *childrenBuffer = NULL;
        ID3D11Buffer *firstBuffer = NULL;
        ID3D11Buffer *secondBuffer = NULL;
        ID3D11Buffer *vertexAppendBuffer = NULL;
        ID3D11Buffer *structureCountBuffer = NULL;
        ID3D11Buffer *computeShaderConstantBuffer = NULL;
        ID3D11ShaderResourceView *nodesBufferSRV = NULL;
		ID3D11ShaderResourceView* childrenBufferSRV = NULL;
        ID3D11ShaderResourceView *vertexAppendBufferSRV = NULL;
        ID3D11UnorderedAccessView *firstBufferUAV = NULL;
        ID3D11UnorderedAccessView *secondBufferUAV = NULL;
        ID3D11UnorderedAccessView *vertexAppendBufferUAV = NULL;
        ComputeShaderConstantBuffer computeShaderConstantBufferData;
    };
}
#endif
