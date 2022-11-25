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
        ID3D11Buffer* bufferPositions = NULL;
        ID3D11Buffer* bufferTextureCoordinates = NULL;
        ID3D11Buffer* bufferNormals = NULL;

        std::vector<ID3D11Resource*> textures;
        std::vector<ID3D11ShaderResourceView*> textureSRVs;
        std::vector<ID3D11Buffer*> triangles;
    };
}
#endif
