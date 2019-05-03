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
        UINT GetStructureCount(ID3D11UnorderedAccessView *UAV);

        int viewMode = 0;
        int vertexBufferCount = 0;
        bool useComputeShader = false;
		bool useViewFrustumCulling = true;

        Octree *octree = NULL;
        SceneObject *text = NULL;
        TextRenderer *textRenderer = NULL;

        // Renderer buffer
        ID3D11Buffer* octreeConstantBuffer = NULL;
        OctreeConstantBuffer octreeConstantBufferData;

		// Blending
		ID3D11DepthStencilView *octreeDepthStencilView = NULL;
		ID3D11Texture2D *octreeDepthStencilTexture = NULL;
		ID3D11ShaderResourceView *octreeDepthTextureSRV = NULL;
		ID3D11ShaderResourceView *octreeStencilTextureSRV = NULL;
		ID3D11BlendState* additiveBlendState = NULL;

        // Compute shader
        ID3D11Buffer *nodesBuffer = NULL;
        ID3D11Buffer *firstBuffer = NULL;
        ID3D11Buffer *secondBuffer = NULL;
        ID3D11Buffer *vertexAppendBuffer = NULL;
        ID3D11Buffer *structureCountBuffer = NULL;
        ID3D11ShaderResourceView *nodesBufferSRV = NULL;
        ID3D11ShaderResourceView *vertexAppendBufferSRV = NULL;
        ID3D11UnorderedAccessView *firstBufferUAV = NULL;
        ID3D11UnorderedAccessView *secondBufferUAV = NULL;
        ID3D11UnorderedAccessView *vertexAppendBufferUAV = NULL;
    };
}
#endif
