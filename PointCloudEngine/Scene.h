#ifndef SCENE_H
#define SCENE_H

#define RENDERER OctreeRenderer

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
        SceneObject *text = NULL;
        SceneObject *pointCloud = NULL;
        TextRenderer *textRenderer = NULL;
        RENDERER *pointCloudRenderer = NULL;
        ConfigFile *configFile = NULL;

        Vector2 input;
        bool rotate = false;
        float cameraPitch = 0;
        float cameraYaw = 0;
        float cameraSpeed = 0;
    };
}
#endif
