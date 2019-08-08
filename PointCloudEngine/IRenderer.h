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
		virtual void SetHelpText(Transform* transform, TextRenderer* textRenderer) = 0;
		virtual void SetText(Transform* transform, TextRenderer* textRenderer) = 0;
		virtual void RemoveComponentFromSceneObject() = 0;
    };
}
#endif
