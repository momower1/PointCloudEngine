#include "PrecompiledHeader.h"
#include "MeshRenderer.h"

MeshRenderer::MeshRenderer(OBJContainer objContainer)
{
    // Create a structured positions buffer description
    D3D11_BUFFER_DESC bufferPositionsDesc;
    ZeroMemory(&bufferPositionsDesc, sizeof(bufferPositionsDesc));
    bufferPositionsDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferPositionsDesc.ByteWidth = sizeof(Vector3) * objContainer.buffers.positions.size();
    bufferPositionsDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    bufferPositionsDesc.StructureByteStride = sizeof(Vector3);
    bufferPositionsDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

    // Fill a D3D11_SUBRESOURCE_DATA struct with the data we want in the buffer
    D3D11_SUBRESOURCE_DATA bufferPositionsData;
    ZeroMemory(&bufferPositionsData, sizeof(bufferPositionsData));
    bufferPositionsData.pSysMem = objContainer.buffers.positions.data();

    // Create the buffer
    hr = d3d11Device->CreateBuffer(&bufferPositionsDesc, &bufferPositionsData, &bufferPositions);
    ERROR_MESSAGE_ON_HR(hr, NAMEOF(d3d11Device->CreateBuffer) + L" failed!");

    // Create a structured texture coordinates buffer description
    D3D11_BUFFER_DESC bufferTextureCoordinatesDesc;
    ZeroMemory(&bufferTextureCoordinatesDesc, sizeof(bufferTextureCoordinatesDesc));
    bufferTextureCoordinatesDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferTextureCoordinatesDesc.ByteWidth = sizeof(Vector2) * objContainer.buffers.textureCoordinates.size();
    bufferTextureCoordinatesDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    bufferTextureCoordinatesDesc.StructureByteStride = sizeof(Vector2);
    bufferTextureCoordinatesDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

    // Fill a D3D11_SUBRESOURCE_DATA struct with the data we want in the buffer
    D3D11_SUBRESOURCE_DATA bufferTextureCoordinatesData;
    ZeroMemory(&bufferTextureCoordinatesData, sizeof(bufferTextureCoordinatesData));
    bufferTextureCoordinatesData.pSysMem = objContainer.buffers.textureCoordinates.data();

    // Create the buffer
    hr = d3d11Device->CreateBuffer(&bufferTextureCoordinatesDesc, &bufferTextureCoordinatesData, &bufferTextureCoordinates);
    ERROR_MESSAGE_ON_HR(hr, NAMEOF(d3d11Device->CreateBuffer) + L" failed!");

    // Create a structured normals buffer description
    D3D11_BUFFER_DESC bufferNormalsDesc;
    ZeroMemory(&bufferNormalsDesc, sizeof(bufferNormalsDesc));
    bufferNormalsDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferNormalsDesc.ByteWidth = sizeof(Vector3) * objContainer.buffers.normals.size();
    bufferNormalsDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    bufferNormalsDesc.StructureByteStride = sizeof(Vector3);
    bufferNormalsDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

    // Fill a D3D11_SUBRESOURCE_DATA struct with the data we want in the buffer
    D3D11_SUBRESOURCE_DATA bufferNormalsData;
    ZeroMemory(&bufferNormalsData, sizeof(bufferNormalsData));
    bufferNormalsData.pSysMem = objContainer.buffers.normals.data();

    // Create the buffer
    hr = d3d11Device->CreateBuffer(&bufferNormalsDesc, &bufferNormalsData, &bufferNormals);
    ERROR_MESSAGE_ON_HR(hr, NAMEOF(d3d11Device->CreateBuffer) + L" failed!");

    for (auto it = objContainer.meshes.begin(); it != objContainer.meshes.end(); it++)
    {
        ID3D11Resource* texture = NULL;
        ID3D11ShaderResourceView* textureSRV = NULL;

        hr = CreateWICTextureFromFile(d3d11Device, d3d11DevCon, it->textureFilename.c_str(), &texture, &textureSRV, 0);
        ERROR_MESSAGE_ON_HR(hr, NAMEOF(CreateWICTextureFromFile) + L" failed!");

        textures.push_back(texture);
        textureSRVs.push_back(textureSRV);

        // Create vertex buffer that stores all the triangles
        D3D11_BUFFER_DESC bufferTrianglesDesc;
        ZeroMemory(&bufferTrianglesDesc, sizeof(bufferTrianglesDesc));
        bufferTrianglesDesc.Usage = D3D11_USAGE_DEFAULT;
        bufferTrianglesDesc.ByteWidth = sizeof(MeshVertex) * it->triangles.size();
        bufferTrianglesDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        // Fill a D3D11_SUBRESOURCE_DATA struct with the data we want in the buffer
        D3D11_SUBRESOURCE_DATA bufferTrianglesData;
        ZeroMemory(&bufferTrianglesData, sizeof(bufferTrianglesData));
        bufferTrianglesData.pSysMem = it->triangles.data();

        // Create the buffer
        ID3D11Buffer* bufferTriangles = NULL;
        hr = d3d11Device->CreateBuffer(&bufferTrianglesDesc, &bufferTrianglesData, &bufferTriangles);
        ERROR_MESSAGE_ON_HR(hr, NAMEOF(d3d11Device->CreateBuffer) + L" failed!");

        triangles.push_back(bufferTriangles);
    }

    std::cout << "DONE" << std::endl;
}

void MeshRenderer::Initialize()
{
}

void MeshRenderer::Update()
{
}

void MeshRenderer::Draw()
{
    // Set structured buffer
    // Set shaders
    // Set input layout
    // Loop
    //      Set current texture
    //      Set current vertex buffer
    //      Draw
}

void MeshRenderer::Release()
{
    SAFE_RELEASE(bufferPositions);
    SAFE_RELEASE(bufferTextureCoordinates);
    SAFE_RELEASE(bufferNormals);

    for (int i = 0; i < textures.size(); i++)
    {
        SAFE_RELEASE(textures[i]);
    }

    for (int i = 0; i < textureSRVs.size(); i++)
    {
        SAFE_RELEASE(textureSRVs[i]);
    }

    for (int i = 0; i < triangles.size(); i++)
    {
        SAFE_RELEASE(triangles[i]);
    }

    textures.clear();
    textureSRVs.clear();
    triangles.clear();
}
