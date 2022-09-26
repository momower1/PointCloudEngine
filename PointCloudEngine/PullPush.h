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
            BOOL isPullPhase;
            XMUINT3 padding;
        };

        int pullPushLevels = 0;
        int pullPushResolution = 0;
        D3D11_SAMPLER_DESC pullPushSamplerDesc;
        PullPushConstantBuffer pullPushConstantBufferData;

        ID3D11Buffer* pullPushConstantBuffer = NULL;
        ID3D11SamplerState* pullPushSamplerState = NULL;
        std::vector<ID3D11Texture2D*> pullPushTextures;
        std::vector<ID3D11UnorderedAccessView*> pullPushTexturesUAV;
        std::vector<ID3D11ShaderResourceView*> pullPushTexturesSRV;
    };
}
#endif

