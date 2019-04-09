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

// Tinyply
#include "tinyply.h"

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
    class SplatRenderer;
    class OctreeRenderer;
    class Settings;
    class Camera;
    class OctreeNode;
    class Octree;
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
#include "DataStructures.h"
#include "Settings.h"
#include "ISetSplatSize.h"
#include "OctreeNode.h"
#include "Octree.h"
#include "TextRenderer.h"
#include "SplatRenderer.h"
#include "OctreeRenderer.h"
#include "Scene.h"

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
extern ID3D11Device* d3d11Device;
extern ID3D11DeviceContext* d3d11DevCon;
extern ID3D11DepthStencilState* depthStencilState;

// Global function declarations
extern void ErrorMessage(std::wstring message, std::wstring header, std::wstring file, int line, HRESULT hr = E_FAIL);
extern void SafeRelease(ID3D11Resource *resource);
extern bool LoadPlyFile(std::vector<Vertex> &vertices, std::wstring plyfile);

// Function declarations
bool InitializeWindow(HINSTANCE hInstancem, int ShowWnd, int width, int hight, bool windowed);
int Messageloop();
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
bool InitializeDirect3d11App(HINSTANCE hInstance);
void ReleaseObjects();
bool InitializeScene();
void UpdateScene();
void DrawScene();

// Template function definitions
template<typename T> void SafeDelete(T*& pointer)
{
    if (pointer != NULL)
    {
        delete pointer;
        pointer = NULL;
    }
}
#endif