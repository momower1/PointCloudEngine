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
        UINT vertexCount = 0;
        UINT triangleCount = 0;
        UINT uvCount = 0;
        UINT normalCount = 0;
        UINT submeshCount = 0;
        UINT textureCount = 0;

        struct MeshRendererConstantBuffer
        {
            Matrix World;
            Matrix View;
            Matrix Projection;
            Matrix WorldInverseTranspose;
            Matrix PreviousWorld;
            Matrix PreviousView;
            Matrix PreviousProjection;
            Matrix PreviousWorldInverseTranspose;
            Vector3 cameraPosition;
            int shadingMode;
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
        std::vector<ID3D11Buffer*> submeshVertices;
        std::vector<UINT> submeshVertexCounts;

        MeshRendererConstantBuffer constantBufferData;
    };
}
#endif
