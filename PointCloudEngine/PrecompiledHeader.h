// FOR NOW IGNORE THE OLD PYTORCH AND HDF5 IMPLEMENTATION
#define IGNORE_OLD_PYTORCH_AND_HDF5_IMPLEMENTATION

#ifndef IGNORE_OLD_PYTORCH_AND_HDF5_IMPLEMENTATION
// Pytorch
#include <torch/script.h>
#include <torch/torch.h>
#endif

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
#include <CommCtrl.h>
#include <shellapi.h>
#include <gdiplus.h>
#include <Shlwapi.h>
#include <ShlObj.h>

// Resources like menus and icons
#include "resource.h"

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

// Cuda Toolkit
#include "cuda.h"
#include "cudaD3D11.h"