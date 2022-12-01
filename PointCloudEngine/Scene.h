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

        // Speed used to increase WASD movement
        Vector2 input;
        float inputSpeed = 0;

        // Waypoint preview variables
        bool waypointPreview;
        float waypointPreviewLocation;
        Vector3 waypointStartPosition;
        Matrix waypointStartRotation;

        struct RenderMode
        {
            std::wstring name;
            ViewMode viewMode;
            ShadingMode shadingMode;
        };

        // Maps from the name of the render mode to the view mode (x) and the shading mode (y)
        std::vector<RenderMode> datasetRenderModes =
        {
            { L"SplatsColor", ViewMode::Splats, ShadingMode::Color },
            { L"SplatsDepth", ViewMode::Splats, ShadingMode::Depth },
            { L"SplatsNormalScreen", ViewMode::Splats, ShadingMode::NormalScreen },

            { L"SplatsSparseColor", ViewMode::SparseSplats, ShadingMode::Color },
            { L"SplatsSparseDepth", ViewMode::SparseSplats, ShadingMode::Depth },
            { L"SplatsSparseNormalScreen", ViewMode::SparseSplats, ShadingMode::NormalScreen },

            { L"PointsColor", ViewMode::Points, ShadingMode::Color },
            { L"PointsDepth", ViewMode::Points, ShadingMode::Depth },
            { L"PointsNormalScreen", ViewMode::Points, ShadingMode::NormalScreen },

            { L"PointsSparseColor", ViewMode::SparsePoints, ShadingMode::Color },
            { L"PointsSparseDepth", ViewMode::SparsePoints, ShadingMode::Depth },
            { L"PointsSparseNormalScreen", ViewMode::SparsePoints, ShadingMode::NormalScreen },

            { L"MeshColor", ViewMode::Mesh, ShadingMode::Color },
            { L"MeshDepth", ViewMode::Mesh, ShadingMode::Depth },
            { L"MeshNormalScreen", ViewMode::Mesh, ShadingMode::NormalScreen },
            { L"MeshOpticalFlowForward", ViewMode::Mesh, ShadingMode::OpticalFlowForward },
            { L"MeshOpticalFlowBackward", ViewMode::Mesh, ShadingMode::OpticalFlowForward },

            { L"PullPushColor", ViewMode::PullPush, ShadingMode::Color },
            { L"PullPushDepth", ViewMode::PullPush, ShadingMode::Depth },
            { L"PullPushNormalScreen", ViewMode::PullPush, ShadingMode::NormalScreen },
        };

        void DrawAndSaveDatasetEntry(UINT index);
    };
}
#endif
