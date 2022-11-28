#ifndef OCTREERENDERER_H
#define OCTREERENDERER_H

#pragma once
#include "PointCloudEngine.h"

namespace PointCloudEngine
{
    class OctreeRenderer : public Component, public IRenderer
    {
    public:
        OctreeRenderer(const std::wstring &pointcloudFile);
        void Initialize();
        void Update();
        void Draw();
        void Release();

        void GetBoundingCubePositionAndSize(Vector3 &outPosition, float &outSize);
		void RemoveComponentFromSceneObject();
        Component* GetComponent();

    private:
        void DrawOctree();
        void DrawOctreeCompute();
        UINT GetStructureCount(ID3D11UnorderedAccessView *UAV);

        int vertexBufferCount = 0;

        Octree *octree = NULL;

        // Renderer buffer
        ID3D11Buffer* octreeConstantBuffer = NULL;
        OctreeConstantBuffer octreeConstantBufferData;

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
