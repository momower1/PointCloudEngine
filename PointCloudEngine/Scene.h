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
        void AddWaypoint();
        void RemoveWaypoint();
        void ToggleWaypoints();
        void PreviewWaypoints();
        void GenerateWaypointDataset();
        void GenerateSphereDataset();

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

		// Speed up WASD for faster movement
        float inputSpeed = 0;

        bool waypointPreview;
        float waypointPreviewLocation;
        Vector3 waypointStartPosition;
        Matrix waypointStartRotation;
    };
}
#endif
