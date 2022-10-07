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
        void Execute(ID3D11Resource* colorTexture, ID3D11ShaderResourceView* depthSRV);
        void Release();
    private:
        struct PullPushConstantBuffer
        {
            int resolutionX;
            int resolutionY;
            int resolutionOutput;
            int pullPushLevel;
            float importanceScale;
            float importanceExponent;
            BOOL isPullPhase;
            BOOL drawImportance;
        };

        int pullPushLevels = 0;
        int pullPushResolution = 0;
        D3D11_SAMPLER_DESC pullPushSamplerDesc;
        PullPushConstantBuffer pullPushConstantBufferData;

        ID3D11Buffer* pullPushConstantBuffer = NULL;
        ID3D11SamplerState* pullPushSamplerState = NULL;
        std::vector<ID3D11Texture2D*> pullPushColorTextures;
        std::vector<ID3D11Texture2D*> pullPushImportanceTextures;
        std::vector<ID3D11ShaderResourceView*> pullPushColorTexturesSRV;
        std::vector<ID3D11UnorderedAccessView*> pullPushColorTexturesUAV;
        std::vector<ID3D11ShaderResourceView*> pullPushImportanceTexturesSRV;
        std::vector<ID3D11UnorderedAccessView*> pullPushImportanceTexturesUAV;

        void CreateTextureResources(D3D11_TEXTURE2D_DESC *textureDesc, ID3D11Texture2D** outTexture, ID3D11ShaderResourceView** outTextureSRV, ID3D11UnorderedAccessView** outTextureUAV);
    };
}
#endif

