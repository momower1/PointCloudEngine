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
			int backfaceCulling;
			float padding[2];
        };

        std::vector<Vertex> vertices;
        GroundTruthRendererConstantBuffer constantBufferData;

        // Vertex buffer
        ID3D11Buffer* vertexBuffer;		        // Holds vertex data
        ID3D11Buffer* constantBuffer;

		// Maps from the name of the render mode to the view mode (x) and the shading mode (y)
		std::map<std::wstring, XMUINT2> renderModes =
		{
			{ L"SplatsColor", XMUINT2(0, 0) },
			{ L"SplatsDepth", XMUINT2(0, 1) },
			{ L"SplatsNormal", XMUINT2(0, 2) },
			{ L"SplatsNormalScreen", XMUINT2(0, 3) },

			{ L"SplatsSparseColor", XMUINT2(1, 0) },
			{ L"SplatsSparseDepth", XMUINT2(1, 1) },
			{ L"SplatsSparseNormal", XMUINT2(1, 2) },
			{ L"SplatsSparseNormalScreen", XMUINT2(1, 3) },

			{ L"PointsColor", XMUINT2(2, 0) },
			{ L"PointsDepth", XMUINT2(2, 1) },
			{ L"PointsNormal", XMUINT2(2, 2) },
			{ L"PointsNormalScreen", XMUINT2(2, 3) },

			{ L"PointsSparseColor", XMUINT2(3, 0) },
			{ L"PointsSparseDepth", XMUINT2(3, 1) },
			{ L"PointsSparseNormal", XMUINT2(3, 2) },
			{ L"PointsSparseNormalScreen", XMUINT2(3, 3) },
		};

		// Used for parsing the model description file
		struct ModelChannel
		{
			std::wstring name;
			torch::Tensor tensor;
			UINT dimensions;
			UINT offset;
			bool normalize;
			bool input;
		};

		// Pytorch Neural Network
		bool loadPytorchModel = true;
		bool validPytorchModel = false;
		torch::jit::script::Module model;
		std::vector<ModelChannel> modelChannels;
		UINT inputDimensions, outputDimensions;
		torch::Tensor inputTensor;
		torch::Tensor outputTensor;
		ID3D11Texture2D* colorTexture = NULL;
		ID3D11Texture2D* depthTexture = NULL;
		float l1Loss, mseLoss, smoothL1Loss;

		// Required to avoid memory overload with the forward function
		// Since we don't use model.backward() it should be fine
		torch::NoGradGuard noGradGuard;

		void DrawNeuralNetwork();
		void CalculateLosses();
		void RenderToTensor(std::wstring renderMode, torch::Tensor& tensor);
		void CopyBackbufferTextureToTensor(torch::Tensor &tensor);
		void CopyDepthTextureToTensor(torch::Tensor &tensor);
		void OutputTensorSize(torch::Tensor &tensor);
		void Redraw(bool present);
		void HDF5DrawDatasets(HDF5File& hdf5file, const UINT groupIndex);
		void GenerateSphereDataset(HDF5File& hdf5file);
		void GenerateWaypointDataset(HDF5File& hdf5file);
		std::vector<std::wstring> SplitString(std::wstring s, wchar_t delimiter);
    };
}
#endif
