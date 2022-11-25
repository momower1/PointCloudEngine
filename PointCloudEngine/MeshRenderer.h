#ifndef MESHRENDERER_H
#define MESHRENDERER_H

#include "PointCloudEngine.h"

namespace PointCloudEngine
{
    class MeshRenderer : public Component
    {
    public:
        MeshRenderer(OBJContainer objContainer);
        void Initialize();
        void Update();
        void Draw();
        void Release();

    private:
        struct MeshRendererConstantBuffer
        {
            Matrix World;
            Matrix View;
            Matrix Projection;
            Matrix WorldInverseTranspose;
            Vector3 cameraPosition;
            float padding;
        };

        ID3D11Buffer* bufferPositions = NULL;
        ID3D11Buffer* bufferTextureCoordinates = NULL;
        ID3D11Buffer* bufferNormals = NULL;
        ID3D11ShaderResourceView* bufferPositionsSRV = NULL;
        ID3D11ShaderResourceView* bufferTextureCoordinatesSRV = NULL;
        ID3D11ShaderResourceView* bufferNormalsSRV = NULL;
        ID3D11Buffer* constantBuffer = NULL;
        ID3D11SamplerState* samplerState = NULL;

        std::vector<ID3D11Resource*> textures;
        std::vector<ID3D11ShaderResourceView*> textureSRVs;
        std::vector<ID3D11Buffer*> triangles;
        std::vector<UINT> triangleVertexCounts;

        MeshRendererConstantBuffer constantBufferData;
    };
}
#endif
