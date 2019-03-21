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

        Vector2 input;
        bool rotate = false;
        float cameraPitch, cameraYaw, cameraSpeed;
    };
}
#endif
