#ifndef SCENE_H
#define SCENE_H

#pragma once
#include "PointCloudEngine.h"

namespace PointCloudEngine
{
    class Scene
    {
    public:
        void Initialize();
        void Update(Timer &timer);
        void Draw();
        void Release();

    private:
        SceneObject *pointCloud;
        SceneObject *fpsText;
        SceneObject *debugText;

        float cameraPitch, cameraYaw, cameraSpeed;
        Vector2 input;
    };
}
#endif
