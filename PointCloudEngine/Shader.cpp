#include "Shader.h"

std::vector<Shader*> Shader::shaders;

// Input layout descriptions
D3D11_INPUT_ELEMENT_DESC Shader::textLayout[] =
{
    {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"OFFSET", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"RECT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
};

D3D11_INPUT_ELEMENT_DESC Shader::splatLayout[] =
{
    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
};

D3D11_INPUT_ELEMENT_DESC Shader::octreeLayout[] =
{
    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"NORMAL", 0, DXGI_FORMAT_R8G8_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"NORMAL", 1, DXGI_FORMAT_R8G8_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"NORMAL", 2, DXGI_FORMAT_R8G8_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"NORMAL", 3, DXGI_FORMAT_R8G8_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"NORMAL", 4, DXGI_FORMAT_R8G8_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"NORMAL", 5, DXGI_FORMAT_R8G8_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"COLOR", 0, DXGI_FORMAT_R16_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"COLOR", 1, DXGI_FORMAT_R16_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"COLOR", 2, DXGI_FORMAT_R16_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"COLOR", 3, DXGI_FORMAT_R16_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"COLOR", 4, DXGI_FORMAT_R16_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"COLOR", 5, DXGI_FORMAT_R16_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"WEIGHTS", 0, DXGI_FORMAT_R32_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"SIZE", 0, DXGI_FORMAT_R32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
};

Shader* Shader::Create(std::wstring filename, bool VS, bool GS, bool PS, bool CS, D3D11_INPUT_ELEMENT_DESC *layout, UINT numElements)
{
    Shader *shader = new Shader(filename, VS, GS, PS, CS, layout, numElements);
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

Shader::Shader(std::wstring filename, bool VS, bool GS, bool PS, bool CS, D3D11_INPUT_ELEMENT_DESC *layout, UINT numElements)
{
    this->VS = VS;
    this->GS = GS;
    this->PS = PS;
    this->CS = CS;

    std::wstring filepath = (executableDirectory + L"/" + filename).c_str();

    // Compile and create the shaders from file
    if (VS)
    {
        ID3DBlob* vertexShaderData = NULL;
        hr = D3DCompileFromFile(filepath.c_str(), 0, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VS", "vs_5_0", 0, 0, &vertexShaderData, 0);
        ErrorMessage(L"D3DCompileFromFile failed for VS of " + filepath, L"Shader", __FILEW__, __LINE__, hr);
        hr = d3d11Device->CreateVertexShader(vertexShaderData->GetBufferPointer(), vertexShaderData->GetBufferSize(), NULL, &vertexShader);
        ErrorMessage(L"CreateVertexShader failed for " + filepath, L"Shader", __FILEW__, __LINE__, hr);

        // Create the Input (Vertex) Layout with numElements being the size of the input layout array
        hr = d3d11Device->CreateInputLayout(layout, numElements, vertexShaderData->GetBufferPointer(), vertexShaderData->GetBufferSize(), &inputLayout);
        ErrorMessage(L"CreateInputLayout failed for " + filepath, L"Shader", __FILEW__, __LINE__, hr);

        SAFE_RELEASE(vertexShaderData);
    }

    if (GS)
    {
        ID3DBlob* geometryShaderData = NULL;
        hr = D3DCompileFromFile(filepath.c_str(), 0, D3D_COMPILE_STANDARD_FILE_INCLUDE, "GS", "gs_5_0", 0, 0, &geometryShaderData, 0);
        ErrorMessage(L"D3DCompileFromFile failed for GS of " + filepath, L"Shader", __FILEW__, __LINE__, hr);
        hr = d3d11Device->CreateGeometryShader(geometryShaderData->GetBufferPointer(), geometryShaderData->GetBufferSize(), NULL, &geometryShader);
        ErrorMessage(L"CreateGeometryShader failed for " + filepath, L"Shader", __FILEW__, __LINE__, hr);
        SAFE_RELEASE(geometryShaderData);
    }

    if (PS)
    {
        ID3DBlob* pixelShaderData = NULL;
        hr = D3DCompileFromFile(filepath.c_str(), 0, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PS", "ps_5_0", 0, 0, &pixelShaderData, 0);
        ErrorMessage(L"D3DCompileFromFile failed for PS of " + filepath, L"Shader", __FILEW__, __LINE__, hr);
        hr = d3d11Device->CreatePixelShader(pixelShaderData->GetBufferPointer(), pixelShaderData->GetBufferSize(), NULL, &pixelShader);
        ErrorMessage(L"CreatePixelShader failed for " + filepath, L"Shader", __FILEW__, __LINE__, hr);
        SAFE_RELEASE(pixelShaderData);
    }

    if (CS)
    {
        ID3DBlob* computeShaderData = NULL;
        hr = D3DCompileFromFile(filepath.c_str(), 0, D3D_COMPILE_STANDARD_FILE_INCLUDE, "CS", "cs_5_0", 0, 0, &computeShaderData, 0);
        ErrorMessage(L"D3DCompileFromFile failed for CS of " + filepath, L"Shader", __FILEW__, __LINE__, hr);
        hr = d3d11Device->CreateComputeShader(computeShaderData->GetBufferPointer(), computeShaderData->GetBufferSize(), NULL, &computeShader);
        ErrorMessage(L"CreateComputeShader failed for " + filepath, L"Shader", __FILEW__, __LINE__, hr);
        SAFE_RELEASE(computeShaderData);
    }
}

void Shader::Release()
{
    SAFE_RELEASE(vertexShader);
    SAFE_RELEASE(geometryShader);
    SAFE_RELEASE(pixelShader);
    SAFE_RELEASE(computeShader);
    SAFE_RELEASE(inputLayout);
}
