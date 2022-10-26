#ifndef PULLPUSH_H
#define PULLPUSH_H

#include "PointCloudEngine.h"

namespace PointCloudEngine
{
    class PullPush
    {
    public:
        PullPush();
        void CreatePullPushTextureHierarchy();
        void SetInitialColorTexture(ID3D11Resource* colorTexture);
        void SetInitialNormalTexture(ID3D11Resource* normalTexture);
        void Execute(ID3D11Resource* backbufferTexture, ID3D11ShaderResourceView* depthSRV);
        void Release();
    private:
        struct PullPushConstantBuffer
        {
            BOOL debug;
            BOOL isPullPhase;
            BOOL orientedSplat;
            BOOL texelBlending;
            int resolutionPullPush;
            int resolutionOutput;
            int resolutionX;
            int resolutionY;
            float blendRange;
            float splatSize;
            int pullPushLevel;
            int padding;
        };

        int pullPushLevels = 0;
        int pullPushResolution = 0;
        D3D11_SAMPLER_DESC pullPushSamplerDesc;
        PullPushConstantBuffer pullPushConstantBufferData;

        ID3D11Buffer* pullPushConstantBuffer = NULL;
        ID3D11SamplerState* pullPushSamplerState = NULL;
        std::vector<ID3D11Texture2D*> pullPushColorTextures;
        std::vector<ID3D11Texture2D*> pullPushNormalTextures;
        std::vector<ID3D11Texture2D*> pullPushPositionTextures;
        std::vector<ID3D11ShaderResourceView*> pullPushColorTexturesSRV;
        std::vector<ID3D11UnorderedAccessView*> pullPushColorTexturesUAV;
        std::vector<ID3D11ShaderResourceView*> pullPushNormalTexturesSRV;
        std::vector<ID3D11UnorderedAccessView*> pullPushNormalTexturesUAV;
        std::vector<ID3D11ShaderResourceView*> pullPushPositionTexturesSRV;
        std::vector<ID3D11UnorderedAccessView*> pullPushPositionTexturesUAV;

        void CreateTextureResources(D3D11_TEXTURE2D_DESC *textureDesc, ID3D11Texture2D** outTexture, ID3D11ShaderResourceView** outTextureSRV, ID3D11UnorderedAccessView** outTextureUAV);
    };
}
#endif

