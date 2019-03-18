#ifndef SHADER_H
#define SHADER_H

#pragma once
#include "PointCloudEngine.h"

class Shader
{
public:
    // Static functions for automatic memory management
    static Shader* Create(std::wstring filename, bool VS, bool GS, bool PS, D3D11_INPUT_ELEMENT_DESC *layout, UINT numElements);
    static void ReleaseAllShaders();

    Shader (std::wstring filename, bool VS, bool GS, bool PS, D3D11_INPUT_ELEMENT_DESC *layout, UINT numElements);
    void Release ();

    static D3D11_INPUT_ELEMENT_DESC textLayout[];

    bool VS, PS, GS;

    // Holds shader information
    ID3D11VertexShader* vertexShader = NULL;
    ID3D11GeometryShader* geometryShader = NULL;
    ID3D11PixelShader* pixelShader = NULL;
    ID3D11InputLayout* inputLayout = NULL;

private:
    static std::vector<Shader*> shaders;

    ID3DBlob* vertexShaderData = NULL;
    ID3DBlob* geometryShaderData = NULL;
    ID3DBlob* pixelShaderData = NULL;
};

#endif