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
			int drawNormals;
			int normalsInScreenSpace;
			float padding[3];
        };

        std::vector<Vertex> vertices;
        GroundTruthRendererConstantBuffer constantBufferData;

        // Vertex buffer
        ID3D11Buffer* vertexBuffer;		        // Holds vertex data
        ID3D11Buffer* constantBuffer;

		std::wstring hdf5DatasetNames[4][4] =
		{
			{ L"SplatsColor", L"SplatsNormal", L"SplatsNormalScreen", L"SplatsDepth" },
			{ L"SplatsSparseColor", L"SplatsSparseNormal", L"SplatsSparseNormalScreen", L"SplatsSparseDepth" },
			{ L"PointsColor", L"PointsNormal", L"PointsNormalScreen", L"PointsDepth" },
			{ L"PointsSparseColor", L"PointsSparseNormal", L"PointsSparseNormalScreen", L"PointsSparseDepth" }
		};

		// Pytorch Neural Network
		bool loadPytorchModel = true;
		torch::jit::script::Module model;
		torch::Tensor colorTensor;
		torch::Tensor depthTensor;
		torch::Tensor inputTensor;
		ID3D11Texture2D* colorTexture = NULL;
		ID3D11Texture2D* depthTexture = NULL;

		// Required to avoid memory overload with the forward function
		// Since we don't use model.backward() it should be fine
		//torch::NoGradGuard noGradGuard;

		void DrawNeuralNetwork();
		void HDF5Draw();
		void HDF5DrawDatasets(HDF5File& hdf5file, const UINT groupIndex);
		void GenerateSphereDataset(HDF5File& hdf5file);
		void GenerateWaypointDataset(HDF5File& hdf5file);
    };
}
#endif
