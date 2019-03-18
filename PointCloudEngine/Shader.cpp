#include "Shader.h"

std::vector<Shader*> Shader::shaders;

// Input layout descriptions
D3D11_INPUT_ELEMENT_DESC Shader::textLayout[] =
{
    {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"OFFSET", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"RECT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
};

Shader* Shader::Create(std::wstring filename, bool VS, bool GS, bool PS, D3D11_INPUT_ELEMENT_DESC *layout, UINT numElements)
{
    Shader *shader = new Shader(filename, VS, GS, PS, layout, numElements);
    shaders.push_back(shader);
    return shader;
}

void Shader::ReleaseAllShaders()
{
    for (auto it = shaders.begin(); it != shaders.end(); it++)
    {
        Shader *shader = *it;
        shader->Release();
        SafeDelete(shader);
    }

    shaders.clear();
}

Shader::Shader(std::wstring filename, bool VS, bool GS, bool PS, D3D11_INPUT_ELEMENT_DESC *layout, UINT numElements)
{
    this->VS = VS;
    this->GS = GS;
    this->PS = PS;

    // Compile and create the shaders from file
    if (VS)
    {
        hr = D3DCompileFromFile(filename.c_str(), 0, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VS", "vs_5_0", 0, 0, &vertexShaderData, 0);
        ErrorMessage(L"D3DCompileFromFile failed for VS of " + filename, L"Shader", __FILEW__, __LINE__, hr);
        hr = d3d11Device->CreateVertexShader(vertexShaderData->GetBufferPointer(), vertexShaderData->GetBufferSize(), NULL, &vertexShader);
        ErrorMessage(L"CreateVertexShader failed for " + filename, L"Shader", __FILEW__, __LINE__, hr);
    }

    if (GS)
    {
        hr = D3DCompileFromFile(filename.c_str(), 0, D3D_COMPILE_STANDARD_FILE_INCLUDE, "GS", "gs_5_0", 0, 0, &geometryShaderData, 0);
        ErrorMessage(L"D3DCompileFromFile failed for GS of " + filename, L"Shader", __FILEW__, __LINE__, hr);
        hr = d3d11Device->CreateGeometryShader(geometryShaderData->GetBufferPointer(), geometryShaderData->GetBufferSize(), NULL, &geometryShader);
        ErrorMessage(L"CreateGeometryShader failed for " + filename, L"Shader", __FILEW__, __LINE__, hr);
    }

    if (PS)
    {
        hr = D3DCompileFromFile(filename.c_str(), 0, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PS", "ps_5_0", 0, 0, &pixelShaderData, 0);
        ErrorMessage(L"D3DCompileFromFile failed for PS of " + filename, L"Shader", __FILEW__, __LINE__, hr);
        hr = d3d11Device->CreatePixelShader(pixelShaderData->GetBufferPointer(), pixelShaderData->GetBufferSize(), NULL, &pixelShader);
        ErrorMessage(L"CreatePixelShader failed for " + filename, L"Shader", __FILEW__, __LINE__, hr);
    }

    // Create the Input (Vertex) Layout with numElements being the size of the input layout array
    hr = d3d11Device->CreateInputLayout(layout, numElements, vertexShaderData->GetBufferPointer(), vertexShaderData->GetBufferSize(), &inputLayout);
    ErrorMessage(L"CreateInputLayout failed for " + filename, L"Shader", __FILEW__, __LINE__, hr);
}

void Shader::Release()
{
    if (VS)
    {
        vertexShader->Release();
        vertexShaderData->Release();
    }

    if (GS)
    {
        geometryShader->Release();
        geometryShaderData->Release();
    }

    if (PS)
    {
        pixelShader->Release();
        pixelShaderData->Release();
    }

    inputLayout->Release();
}
