#ifndef POINTCLOUDENGINE_H
#define POINTCLOUDENGINE_H

#include "PrecompiledHeader.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

// Forward declarations
class HDF5File;

namespace PointCloudEngine
{
    class SceneObject;
    class Shader;
    class Transform;
    class TextRenderer;
    class GroundTruthRenderer;
    class OctreeRenderer;
	class WaypointRenderer;
    class Settings;
    class Camera;
    class Octree;
	class GUI;
    struct OctreeNode;

	enum class ViewMode
	{
		OctreeSplats,
		OctreeNodes,
		OctreeNormalClusters,
		Splats,
		SparseSplats,
		Points,
		SparsePoints,
		NeuralNetwork
	};

	enum class ShadingMode
	{
		Color,
		Depth,
		Normal,
		NormalScreen
	};
}

using namespace PointCloudEngine;

#include "Transform.h"
#include "Camera.h"
#include "Input.h"
#include "Shader.h"
#include "Timer.h"
#include "Component.h"
#include "SceneObject.h"
#include "Hierarchy.h"
#include "Structures.h"
#include "Settings.h"
#include "IRenderer.h"
#include "OctreeNode.h"
#include "Octree.h"
#include "TextRenderer.h"
#include "GroundTruthRenderer.h"
#include "OctreeRenderer.h"
#include "WaypointRenderer.h"
#include "GUI.h"
#include "Scene.h"
#include "HDF5File.h"

// Preprocessor macros
#define NAMEOF(variable) std::wstring(L#variable)
#define ERROR_MESSAGE(message) ErrorMessageOnFail(E_FAIL, message, __FILEW__, __LINE__)
#define ERROR_MESSAGE_ON_FAIL(hr, message) ErrorMessageOnFail(hr, message, __FILEW__, __LINE__)
#define SAFE_RELEASE(resource) if((resource) != NULL) { (resource)->Release(); (resource) = NULL; }

// Template function definitions
template<typename T> void SafeDelete(T*& pointer)
{
	if (pointer != NULL)
	{
		delete pointer;
		pointer = NULL;
	}
}

// Global variables, accessable in other files
extern std::wstring executablePath;
extern std::wstring executableDirectory;
extern double dt;
extern HRESULT hr;
extern HWND hwnd;
extern Settings* settings;
extern Camera* camera;
extern Shader* textShader;
extern Shader* splatShader;
extern Shader* pointShader;
extern Shader* waypointShader;
extern Shader* octreeCubeShader;
extern Shader* octreeSplatShader;
extern Shader* octreeClusterShader;
extern Shader* octreeComputeShader;
extern Shader* octreeComputeVSShader;
extern Shader* blendingShader;
extern Shader* textureConversionShader;
extern IDXGISwapChain* swapChain;
extern ID3D11Device* d3d11Device;
extern ID3D11DeviceContext* d3d11DevCon;
extern ID3D11RenderTargetView* renderTargetView;
extern ID3D11Texture2D* backBufferTexture;
extern ID3D11UnorderedAccessView* backBufferTextureUAV;
extern ID3D11Texture2D* depthStencilTexture;
extern ID3D11DepthStencilView* depthStencilView;
extern ID3D11ShaderResourceView* depthTextureSRV;
extern ID3D11DepthStencilState* depthStencilState;
extern ID3D11BlendState* blendState;
extern ID3D11Buffer* nullBuffer[1];
extern ID3D11UnorderedAccessView* nullUAV[1];
extern ID3D11ShaderResourceView* nullSRV[1];
extern ID3D11Buffer* lightingConstantBuffer;
extern LightingConstantBuffer lightingConstantBufferData;

// Global function declarations
extern void ErrorMessageOnFail(HRESULT hr, std::wstring message, std::wstring file, int line);
extern bool LoadPointcloudFile(std::vector<Vertex> &outVertices, Vector3 &outBoundingCubePosition, float &outBoundingCubeSize, const std::wstring &pointcloudFile);
extern void SaveScreenshotToFile();
extern void SetFullscreen(bool fullscreen);
extern void DrawBlended(UINT vertexCount, ID3D11Buffer* constantBuffer, const void* constantBufferData, int &useBlending);

// Function declarations
bool InitializeWindow(HINSTANCE hInstance, int ShowWnd);
int Messageloop();
LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
bool InitializeDirect3d11App(HINSTANCE hInstance);
void ReleaseObjects();
bool InitializeScene();
void UpdateScene();
void DrawScene();

#endif