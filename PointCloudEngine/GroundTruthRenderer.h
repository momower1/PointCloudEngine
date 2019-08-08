#ifndef GROUNDTRUTHRENDERER_H
#define GROUNDTRUTHRENDERER_H

#pragma once
#include "PointCloudEngine.h"

namespace PointCloudEngine
{
    class GroundTruthRenderer : public Component, public IRenderer
    {
    public:
        GroundTruthRenderer(const std::wstring &pointcloudFile);
        void Initialize();
        void Update();
        void Draw();
        void Release();

        void GetBoundingCubePositionAndSize(Vector3 &outPosition, float &outSize);
		void SetHelpText(Transform* helpTextTransform, TextRenderer* helpTextRenderer);
		void SetText(Transform* textTransform, TextRenderer* textRenderer);
		void RemoveComponentFromSceneObject();

    private:
		Vector3 boundingCubePosition;
		float boundingCubeSize;

        // Same constant buffer as in effect file, keep packing rules in mind
        struct GroundTruthRendererConstantBuffer
        {
            Matrix World;
            Matrix View;
            Matrix Projection;
            Matrix WorldInverseTranspose;
			Matrix WorldViewProjectionInverse;
            Vector3 cameraPosition;
            float fovAngleY;
            float samplingRate;
			float blendFactor;
			int useBlending;
			float padding;
        };

        std::vector<Vertex> vertices;
        GroundTruthRendererConstantBuffer constantBufferData;

        // Vertex buffer
        ID3D11Buffer* vertexBuffer;		        // Holds vertex data
        ID3D11Buffer* constantBuffer;
    };
}
#endif
