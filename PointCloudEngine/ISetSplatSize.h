#ifndef ISETSPLATSIZE_H
#define ISETSPLATSIZE_H

#pragma once
#include "PointCloudEngine.h"

namespace PointCloudEngine
{
    class ISetSplatSize
    {
    public:
        // Splat size is in screen size between 1 (whole screen) and 1.0f/resolutionY (one pixel)
        virtual void SetSplatSize(const float &splatSize) = 0;
    };
}
#endif
