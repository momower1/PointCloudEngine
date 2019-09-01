#ifndef WAYPOINTRENDERER_H
#define WAYPOINTRENDERER_H

#pragma once
#include "PointCloudEngine.h"

namespace PointCloudEngine
{
    class WaypointRenderer : public Component
    {
    public:
        void Initialize();
        void Update();
        void Draw();
        void Release();

    private:
		struct WaypointRendererConstantBuffer
        {
            Matrix View;
            Matrix Projection;
        };

		std::vector<Vector3> waypointPositions;
		std::vector<Vector3> waypointForwards;

        std::vector<Vector3> vertices;
        WaypointRendererConstantBuffer constantBufferData;

        ID3D11Buffer* vertexBuffer = NULL;
        ID3D11Buffer* constantBuffer = NULL;

		void UpdateVertexBuffer();
    };
}
#endif
