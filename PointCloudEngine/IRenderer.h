#ifndef IRENDERER_H
#define IRENDERER_H

#pragma once
#include "PointCloudEngine.h"

namespace PointCloudEngine
{
    class IRenderer
    {
    public:
        virtual void GetBoundingCubePositionAndSize(Vector3 &outPosition, float &outSize) = 0;
		virtual void RemoveComponentFromSceneObject() = 0;
        virtual Component* GetComponent() = 0;
    };
}
#endif
