#ifndef IRENDERER_H
#define IRENDERER_H

#pragma once
#include "PointCloudEngine.h"

namespace PointCloudEngine
{
    class IRenderer
    {
    public:
        // Splat size is in screen size between 1 (whole screen) and 1.0f/resolutionY (one pixel)
        virtual void SetSplatSize(const float &splatSize) = 0;
        virtual void GetBoundingCubePositionAndSize(Vector3 &outPosition, float &outSize) = 0;
    };
}
#endif
