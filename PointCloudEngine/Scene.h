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
		void OpenPlyOrPointcloudFile();
		void LoadFile(std::wstring filepath);

    private:
		SceneObject *startupText = NULL;
        SceneObject *loadingText = NULL;
        SceneObject *pointCloud = NULL;
		TextRenderer* startupTextRenderer = NULL;
		TextRenderer* loadingTextRenderer = NULL;
		WaypointRenderer* waypointRenderer = NULL;
        MeshRenderer* meshRenderer = NULL;
        IRenderer *pointCloudRenderer = NULL;

        Vector2 input;

		// Speed up WASD, Q/E, V/N and so on for faster movement and parameter tweaking
        float inputSpeed = 0;
    };
}
#endif
