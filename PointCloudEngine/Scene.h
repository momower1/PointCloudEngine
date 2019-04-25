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
        void DelayedLoadFile(std::wstring filepath);
        void LoadFile();

        SceneObject *text = NULL;
        SceneObject *loadingText = NULL;
        SceneObject *pointCloud = NULL;
        TextRenderer *textRenderer = NULL;
        RENDERER *pointCloudRenderer = NULL;

        Vector2 input;
        bool help = false;
        bool rotate = false;
        float splatSize = 0.01f;
        float cameraPitch = 0;
        float cameraYaw = 0;
        float cameraSpeed = 0;

        float timeUntilLoadFile = -1.0f;
        float timeSinceLoadFile = 0.0f;

		Vector3 cameraPositions[6] =
		{
			Vector3(0, 0, -10),
			Vector3(0, 0, 10),
			Vector3(-10, 0, 0),
			Vector3(10, 0, 0),
			Vector3(0, 10, 0),
			Vector3(0, -10, 0)
		};

		Vector2 cameraPitchYaws[6] =
		{
			Vector2(0, 0),
			Vector2(0, XM_PI),
			Vector2(0, XM_PI / 2),
			Vector2(0, -XM_PI / 2),
			Vector2(XM_PI / 2, 0),
			Vector2(-XM_PI / 2, 0)
		};
    };
}
#endif
