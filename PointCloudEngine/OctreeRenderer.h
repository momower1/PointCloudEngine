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
        void Initialize();
        void Update();
        void Draw();
        void Release();

        void GetBoundingCubePositionAndSize(Vector3 &outPosition, float &outSize);
		void RemoveComponentFromSceneObject();

    private:
        void DrawOctree();
        void DrawOctreeCompute();
		void DrawOctreeBlended();
        UINT GetStructureCount(ID3D11UnorderedAccessView *UAV);

        int viewMode = 0;
        int vertexBufferCount = 0;
        bool useComputeShader = true;
		bool help = false;

        Octree *octree = NULL;
        SceneObject *text = NULL;
		SceneObject* helpText = NULL;
        TextRenderer *textRenderer = NULL;
		TextRenderer* helpTextRenderer = NULL;

        // Renderer buffer
        ID3D11Buffer* octreeConstantBuffer = NULL;
        OctreeConstantBuffer octreeConstantBufferData;

		// Blending
		ID3D11DepthStencilView *octreeDepthStencilView = NULL;
		ID3D11Texture2D *octreeDepthTexture = NULL;
		ID3D11ShaderResourceView *octreeDepthTextureSRV = NULL;
		ID3D11BlendState* additiveBlendState = NULL;
		ID3D11DepthStencilState* depthTestDisabledDepthStencilState = NULL;

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

		// This is used to unbind buffers and views from the shaders
		ID3D11Buffer* nullBuffer[1] = { NULL };
		ID3D11UnorderedAccessView* nullUAV[1] = { NULL };
		ID3D11ShaderResourceView* nullSRV[1] = { NULL };
    };
}
#endif
