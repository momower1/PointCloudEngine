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
        void Execute(ID3D11UnorderedAccessView* initialColorUAV, ID3D11DepthStencilView* initialDepthView);
        void Release();
    private:
        int pullPushLevels = 0;
        int pullPushResolution = 0;
        std::vector<ID3D11Texture2D*> pullTextures;
        std::vector<ID3D11Texture2D*> pushTextures;
        std::vector<ID3D11UnorderedAccessView*> pullTexturesUAV;
        std::vector<ID3D11UnorderedAccessView*> pushTexturesUAV;
    };
}
#endif

