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
		void UpdateConstantBuffer();
		void UpdatePreviousMatrices();
		void LoadSurfaceClassificationModel();
		void LoadSurfaceFlowModel();
		void LoadSurfaceReconstructionModel();

        void GetBoundingCubePositionAndSize(Vector3 &outPosition, float &outSize);
		void RemoveComponentFromSceneObject();
		Component* GetComponent();

#ifndef IGNORE_OLD_PYTORCH_AND_HDF5_IMPLEMENTATION
		void GenerateSphereDataset();
		void GenerateWaypointDataset();
		void LoadNeuralNetworkPytorchModel();
		void LoadNeuralNetworkDescriptionFile();
		void ApplyNeuralNetworkResolution();
#endif

    private:
		Vector3 boundingCubePosition;
		float boundingCubeSize;

        std::vector<Vertex> vertices;
        GroundTruthConstantBuffer constantBufferData;

        // Vertex buffer
        ID3D11Buffer* vertexBuffer;		        // Holds vertex data
        ID3D11Buffer* constantBuffer;

		// Pull push algorithm
		PullPush* pullPush = NULL;

		// Neural network
		bool validSCM = false;
		bool validSFM = false;
		bool validSRM = false;
		torch::jit::script::Module SCM;
		torch::jit::script::Module SFM;
		torch::jit::script::Module SRM;
		torch::Tensor previousOutputSRM;

		// Required to avoid memory overload with the forward function
		// Since we don't use model.backward() it should be fine
		torch::NoGradGuard noGradGuard;

		void Redraw(bool present);
		void DrawNeuralNetwork();
		torch::Tensor NormalizeDepthTensor(torch::Tensor& depthTensor, torch::Tensor& foregroundMask, torch::Tensor& backgroundMask);
		std::tuple<torch::Tensor, torch::Tensor, torch::Tensor> ConvertTensorIntoZeroToOneRange(torch::Tensor& tensorFull);
		torch::Tensor RevertTensorIntoFullRange(torch::Tensor& tensorMin, torch::Tensor& tensorMax, torch::Tensor& tensorZeroOne);
		torch::Tensor WarpImage(torch::Tensor& imageTensor, torch::Tensor& motionVectorTensor);
		bool LoadNeuralNetworkModel(std::wstring& filename, torch::jit::script::Module& model);
		torch::Tensor EvaluateNeuralNetworkModel(torch::jit::script::Module& model, torch::Tensor inputTensor);

#ifndef IGNORE_OLD_PYTORCH_AND_HDF5_IMPLEMENTATION
		// Maps from the name of the render mode to the view mode (x) and the shading mode (y)
		std::map<std::wstring, XMUINT2> renderModes =
		{
			{ L"SplatsColor", XMUINT2((UINT)ViewMode::Splats, (UINT)ShadingMode::Color) },
			{ L"SplatsDepth", XMUINT2((UINT)ViewMode::Splats, (UINT)ShadingMode::Depth) },
			{ L"SplatsNormal", XMUINT2((UINT)ViewMode::Splats, (UINT)ShadingMode::Normal) },
			{ L"SplatsNormalScreen", XMUINT2((UINT)ViewMode::Splats, (UINT)ShadingMode::NormalScreen) },

			{ L"SplatsSparseColor", XMUINT2((UINT)ViewMode::SparseSplats, (UINT)ShadingMode::Color) },
			{ L"SplatsSparseDepth", XMUINT2((UINT)ViewMode::SparseSplats, (UINT)ShadingMode::Depth) },
			{ L"SplatsSparseNormal", XMUINT2((UINT)ViewMode::SparseSplats, (UINT)ShadingMode::Normal) },
			{ L"SplatsSparseNormalScreen", XMUINT2((UINT)ViewMode::SparseSplats, (UINT)ShadingMode::NormalScreen) },

			{ L"PointsColor", XMUINT2((UINT)ViewMode::Points, (UINT)ShadingMode::Color) },
			{ L"PointsDepth", XMUINT2((UINT)ViewMode::Points, (UINT)ShadingMode::Depth) },
			{ L"PointsNormal", XMUINT2((UINT)ViewMode::Points, (UINT)ShadingMode::Normal) },
			{ L"PointsNormalScreen", XMUINT2((UINT)ViewMode::Points, (UINT)ShadingMode::NormalScreen) },

			{ L"PointsSparseColor", XMUINT2((UINT)ViewMode::SparsePoints, (UINT)ShadingMode::Color) },
			{ L"PointsSparseDepth", XMUINT2((UINT)ViewMode::SparsePoints, (UINT)ShadingMode::Depth) },
			{ L"PointsSparseNormal", XMUINT2((UINT)ViewMode::SparsePoints, (UINT)ShadingMode::Normal) },
			{ L"PointsSparseNormalScreen", XMUINT2((UINT)ViewMode::SparsePoints, (UINT)ShadingMode::NormalScreen) },
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
		bool validDescriptionFile = false;
		bool createInputOutputTensors = true;
		torch::jit::script::Module model;
		std::vector<ModelChannel> modelChannels;
		UINT inputDimensions, outputDimensions;
		torch::Tensor inputTensor;
		torch::Tensor outputTensor;
		ID3D11Texture2D* colorTexture = NULL;
		ID3D11Texture2D* depthTexture = NULL;

		// Required to avoid memory overload with the forward function
		// Since we don't use model.backward() it should be fine
		torch::NoGradGuard noGradGuard;

		void CalculateLosses();
		void RenderToTensor(std::wstring renderMode, torch::Tensor& tensor);
		void CopyBackbufferTextureToTensor(torch::Tensor &tensor);
		void CopyDepthTextureToTensor(torch::Tensor &tensor);
		void OutputTensorSize(torch::Tensor &tensor);
		void HDF5DrawDatasets(HDF5File& hdf5file, const UINT groupIndex);
		void HDF5DrawRenderModes(HDF5File& hdf5file, H5::Group group, std::wstring comment, bool sparseUpsample = false);
		HDF5File CreateDatasetHDF5File();
		std::vector<std::wstring> SplitString(std::wstring s, wchar_t delimiter);
#endif
    };
}
#endif
