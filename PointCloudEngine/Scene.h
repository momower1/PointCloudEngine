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
        void DelayedLoadFile(std::wstring filepath);
        void LoadFile();

		SceneObject* text = NULL;
		SceneObject* helpText = NULL;
		SceneObject* fpsText = NULL;
		SceneObject *startupText = NULL;
        SceneObject *loadingText = NULL;
        SceneObject *pointCloud = NULL;
		TextRenderer* textRenderer = NULL;
		TextRenderer* helpTextRenderer = NULL;
		TextRenderer *fpsTextRenderer = NULL;
		TextRenderer* startupTextRenderer = NULL;
		TextRenderer* loadingTextRenderer = NULL;
		WaypointRenderer* waypointRenderer = NULL;
        IRenderer *pointCloudRenderer = NULL;

        Vector2 input;
        float cameraPitch = 0;
        float cameraYaw = 0;

		// Waypoint camera trackshot
		float waypointPreviewLocation = 0;
		Vector3 waypointStartPosition;

		// Speed up WASD, Q/E, V/N and so on for faster movement and parameter tweaking
        float inputSpeed = 0;

		// Rendering parameters that can be changed in both ground truth and octree renderer
		ID3D11Buffer* lightingConstantBuffer = NULL;
		LightingConstantBuffer lightingConstantBufferData;

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
