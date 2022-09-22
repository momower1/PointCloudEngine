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
        void Execute(ID3D11UnorderedAccessView* colorUAV, ID3D11ShaderResourceView* depthSRV);
        void Release();
    private:
        struct PullPushConstantBuffer
        {
            int resolutionX;
            int resolutionY;
            int pullPushLevel;
            BOOL isPullPhase;
        };

        int pullPushLevels = 0;
        int pullPushResolution = 0;
        PullPushConstantBuffer pullPushConstantBufferData;

        ID3D11Buffer* pullPushConstantBuffer;
        std::vector<ID3D11Texture2D*> pullTextures;
        std::vector<ID3D11Texture2D*> pushTextures;
        std::vector<ID3D11UnorderedAccessView*> pullTexturesUAV;
        std::vector<ID3D11UnorderedAccessView*> pushTexturesUAV;
    };
}
#endif

