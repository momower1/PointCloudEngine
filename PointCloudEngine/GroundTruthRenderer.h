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
    };
}
#endif
