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
			int normal;
        };

        std::vector<Vertex> vertices;
        GroundTruthRendererConstantBuffer constantBufferData;

        // Vertex buffer
        ID3D11Buffer* vertexBuffer;		        // Holds vertex data
        ID3D11Buffer* constantBuffer;

		std::wstring hdf5DatasetNames[4][3] =
		{
			{ L"SplatsColor", L"SplatsNormal", L"SplatsDepth" },
			{ L"SplatsSparseColor", L"SplatsSparseNormal", L"SplatsSparseDepth" },
			{ L"PointsColor", L"PointsNormal", L"PointsDepth" },
			{ L"PointsSparseColor", L"PointsSparseNormal", L"PointsSparseDepth" }
		};

		void HDF5Draw();
		void HDF5DrawDatasets(HDF5File& hdf5file, const UINT groupIndex);
		void GenerateSphereDataset(HDF5File& hdf5file);
		void GenerateWaypointDataset(HDF5File& hdf5file);
    };
}
#endif
