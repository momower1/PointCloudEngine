#ifndef POINTCLOUDENGINE_H
#define POINTCLOUDENGINE_H

#pragma once
#include <windows.h>
#include <windowsx.h>
#include <d3d11.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include <commdlg.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <comdef.h>
#include <iomanip>
#include <algorithm>
#include <limits>
#include <map>
#include <queue>
#include <math.h>
#include <wincodec.h>

// DirectX Toolkit
#include "CommonStates.h"
#include "DDSTextureLoader.h"
#include "DirectXHelpers.h"
#include "Effects.h"
#include "GamePad.h"
#include "GeometricPrimitive.h"
#include "GraphicsMemory.h"
#include "Keyboard.h"
#include "Model.h"
#include "Mouse.h"
#include "PrimitiveBatch.h"
#include "ScreenGrab.h"
#include "SimpleMath.h"
#include "SpriteBatch.h"
#include "SpriteFont.h"
#include "VertexTypes.h"
#include "WICTextureLoader.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

// Forward declarations
namespace PointCloudEngine
{
    class SceneObject;
    class Shader;
    class Transform;
    class TextRenderer;
    class GroundTruthRenderer;
    class OctreeRenderer;
    class Settings;
    class Camera;
    class Octree;
    struct OctreeNode;
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
#include "Scene.h"

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
extern Shader* octreeCubeShader;
extern Shader* octreeSplatShader;
extern Shader* octreeClusterShader;
extern Shader* octreeComputeShader;
extern Shader* octreeComputeVSShader;
extern Shader* blendingShader;
extern ID3D11Device* d3d11Device;
extern ID3D11DeviceContext* d3d11DevCon;
extern ID3D11RenderTargetView* renderTargetView;
extern ID3D11Texture2D* backBufferTexture;
extern ID3D11UnorderedAccessView* backBufferTextureUAV;
extern ID3D11DepthStencilView* depthStencilView;
extern ID3D11ShaderResourceView* depthTextureSRV;
extern ID3D11DepthStencilState* depthStencilState;
extern ID3D11BlendState* blendState;
extern ID3D11Buffer* nullBuffer[1];
extern ID3D11UnorderedAccessView* nullUAV[1];
extern ID3D11ShaderResourceView* nullSRV[1];

// Global function declarations
extern void ErrorMessageOnFail(HRESULT hr, std::wstring message, std::wstring file, int line);
extern bool LoadPointcloudFile(std::vector<Vertex> &vertices, const std::wstring &pointcloudFile);
extern void SaveScreenshotToFile();
extern void SetFullscreen(bool fullscreen);
extern void DrawBlended(UINT vertexCount, ID3D11Buffer* constantBuffer, const void* constantBufferData, int &useBlending);

// Function declarations
bool InitializeWindow(HINSTANCE hInstancem, int ShowWnd, int width, int hight, bool windowed);
int Messageloop();
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
bool InitializeDirect3d11App(HINSTANCE hInstance);
void ReleaseObjects();
bool InitializeScene();
void UpdateScene();
void DrawScene();

#endif