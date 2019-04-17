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

    // Compile and create the shaders from file, the shader functions have to be named VS, GS, PS and CS for this to work
    if (VS)
    {
        ID3DBlob* vertexShaderData = NULL;
        hr = D3DCompileFromFile(filepath.c_str(), 0, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VS", "vs_5_0", 0, 0, &vertexShaderData, 0);
		ERROR_MESSAGE_ON_FAIL(hr, NAMEOF(D3DCompileFromFile) + L" failed for the VS of " + filepath);
        hr = d3d11Device->CreateVertexShader(vertexShaderData->GetBufferPointer(), vertexShaderData->GetBufferSize(), NULL, &vertexShader);
		ERROR_MESSAGE_ON_FAIL(hr, NAMEOF(d3d11Device->CreateVertexShader) + L" failed for the VS of " + filepath);

        if (numElements > 0)
        {
            // Create the Input (Vertex) Layout with numElements being the size of the input layout array
            hr = d3d11Device->CreateInputLayout(layout, numElements, vertexShaderData->GetBufferPointer(), vertexShaderData->GetBufferSize(), &inputLayout);
			ERROR_MESSAGE_ON_FAIL(hr, NAMEOF(d3d11Device->CreateInputLayout) + L" failed for the VS of " + filepath);
        }

        SAFE_RELEASE(vertexShaderData);
    }

    if (GS)
    {
        ID3DBlob* geometryShaderData = NULL;
        hr = D3DCompileFromFile(filepath.c_str(), 0, D3D_COMPILE_STANDARD_FILE_INCLUDE, "GS", "gs_5_0", 0, 0, &geometryShaderData, 0);
		ERROR_MESSAGE_ON_FAIL(hr, NAMEOF(D3DCompileFromFile) + L" failed for the GS of " + filepath);
        hr = d3d11Device->CreateGeometryShader(geometryShaderData->GetBufferPointer(), geometryShaderData->GetBufferSize(), NULL, &geometryShader);
		ERROR_MESSAGE_ON_FAIL(hr, NAMEOF(d3d11Device->CreateGeometryShader) + L" failed for the GS of " + filepath);
        SAFE_RELEASE(geometryShaderData);
    }

    if (PS)
    {
        ID3DBlob* pixelShaderData = NULL;
        hr = D3DCompileFromFile(filepath.c_str(), 0, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PS", "ps_5_0", 0, 0, &pixelShaderData, 0);
		ERROR_MESSAGE_ON_FAIL(hr, NAMEOF(D3DCompileFromFile) + L" failed for the PS of " + filepath);
        hr = d3d11Device->CreatePixelShader(pixelShaderData->GetBufferPointer(), pixelShaderData->GetBufferSize(), NULL, &pixelShader);
		ERROR_MESSAGE_ON_FAIL(hr, NAMEOF(d3d11Device->CreatePixelShader) + L" failed for the PS of " + filepath);
        SAFE_RELEASE(pixelShaderData);
    }

    if (CS)
    {
        ID3DBlob* computeShaderData = NULL;
        hr = D3DCompileFromFile(filepath.c_str(), 0, D3D_COMPILE_STANDARD_FILE_INCLUDE, "CS", "cs_5_0", 0, 0, &computeShaderData, 0);
		ERROR_MESSAGE_ON_FAIL(hr, NAMEOF(D3DCompileFromFile) + L" failed for the CS of " + filepath);
        hr = d3d11Device->CreateComputeShader(computeShaderData->GetBufferPointer(), computeShaderData->GetBufferSize(), NULL, &computeShader);
		ERROR_MESSAGE_ON_FAIL(hr, NAMEOF(d3d11Device->CreateComputeShader) + L" failed for the CS of " + filepath);
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
