#ifndef SHADER_H
#define SHADER_H

#pragma once
#include "PointCloudEngine.h"

namespace PointCloudEngine
{
    class Shader
    {
    public:
        // Static functions for automatic memory management
        static Shader* Create(std::wstring filename, bool VS, bool GS, bool PS, bool CS, D3D11_INPUT_ELEMENT_DESC *layout, UINT numElements);
        static void ReleaseAllShaders();

        Shader (std::wstring filename, bool VS, bool GS, bool PS, bool CS, D3D11_INPUT_ELEMENT_DESC *layout, UINT numElements);
        void Release ();

        static D3D11_INPUT_ELEMENT_DESC textLayout[];
        static D3D11_INPUT_ELEMENT_DESC splatLayout[];
        static D3D11_INPUT_ELEMENT_DESC octreeLayout[];
		static D3D11_INPUT_ELEMENT_DESC waypointLayout[];
        static D3D11_INPUT_ELEMENT_DESC meshLayout[];

        bool VS, PS, GS, CS;

        // Holds shader information
        ID3D11VertexShader* vertexShader = NULL;
        ID3D11GeometryShader* geometryShader = NULL;
        ID3D11PixelShader* pixelShader = NULL;
        ID3D11ComputeShader* computeShader = NULL;
        ID3D11InputLayout* inputLayout = NULL;

    private:
        static std::vector<Shader*> shaders;

		std::wstring ToWstring(ID3DBlob* compilerMessages);
    };
}
#endif