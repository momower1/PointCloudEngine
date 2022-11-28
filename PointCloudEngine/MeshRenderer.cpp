#include "PrecompiledHeader.h"
#include "MeshRenderer.h"

MeshRenderer::MeshRenderer(OBJContainer objContainer)
{
    // Positions
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

        // Create the shader resource view
        D3D11_SHADER_RESOURCE_VIEW_DESC bufferPositionsSRVDesc;
        ZeroMemory(&bufferPositionsSRVDesc, sizeof(bufferPositionsSRVDesc));
        bufferPositionsSRVDesc.Format = DXGI_FORMAT_UNKNOWN;
        bufferPositionsSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
        bufferPositionsSRVDesc.Buffer.ElementWidth = sizeof(Vector3);
        bufferPositionsSRVDesc.Buffer.NumElements = objContainer.buffers.positions.size();

        hr = d3d11Device->CreateShaderResourceView(bufferPositions, &bufferPositionsSRVDesc, &bufferPositionsSRV);
        ERROR_MESSAGE_ON_HR(hr, NAMEOF(d3d11Device->CreateShaderResourceView) + L" failed!");
    }

    // Texture coordinates
    {
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

        // Create the shader resource view
        D3D11_SHADER_RESOURCE_VIEW_DESC bufferTextureCoordinatesSRVDesc;
        ZeroMemory(&bufferTextureCoordinatesSRVDesc, sizeof(bufferTextureCoordinatesSRVDesc));
        bufferTextureCoordinatesSRVDesc.Format = DXGI_FORMAT_UNKNOWN;
        bufferTextureCoordinatesSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
        bufferTextureCoordinatesSRVDesc.Buffer.ElementWidth = sizeof(Vector2);
        bufferTextureCoordinatesSRVDesc.Buffer.NumElements = objContainer.buffers.textureCoordinates.size();

        hr = d3d11Device->CreateShaderResourceView(bufferTextureCoordinates, &bufferTextureCoordinatesSRVDesc, &bufferTextureCoordinatesSRV);
        ERROR_MESSAGE_ON_HR(hr, NAMEOF(d3d11Device->CreateShaderResourceView) + L" failed!");
    }
    
    // Normals
    {
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

        // Create the shader resource view
        D3D11_SHADER_RESOURCE_VIEW_DESC bufferNormalsSRVDesc;
        ZeroMemory(&bufferNormalsSRVDesc, sizeof(bufferNormalsSRVDesc));
        bufferNormalsSRVDesc.Format = DXGI_FORMAT_UNKNOWN;
        bufferNormalsSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
        bufferNormalsSRVDesc.Buffer.ElementWidth = sizeof(Vector3);
        bufferNormalsSRVDesc.Buffer.NumElements = objContainer.buffers.normals.size();

        hr = d3d11Device->CreateShaderResourceView(bufferNormals, &bufferNormalsSRVDesc, &bufferNormalsSRV);
        ERROR_MESSAGE_ON_HR(hr, NAMEOF(d3d11Device->CreateShaderResourceView) + L" failed!");
    }

    // Textures
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
        triangleVertexCounts.push_back(it->triangles.size());
    }

    // Create the constant buffer
    D3D11_BUFFER_DESC constantBufferDesc;
    ZeroMemory(&constantBufferDesc, sizeof(constantBufferDesc));
    constantBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    constantBufferDesc.ByteWidth = sizeof(MeshRendererConstantBuffer);
    constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    hr = d3d11Device->CreateBuffer(&constantBufferDesc, NULL, &constantBuffer);
    ERROR_MESSAGE_ON_HR(hr, NAMEOF(d3d11Device->CreateBuffer) + L" failed!");

    // Create the texture sampler state
    D3D11_SAMPLER_DESC samplerDesc;
    ZeroMemory(&samplerDesc, sizeof(samplerDesc));
    samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    samplerDesc.MaxAnisotropy = 16;
    samplerDesc.MinLOD = 0;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    // Create the Sample State
    hr = d3d11Device->CreateSamplerState(&samplerDesc, &samplerState);
    ERROR_MESSAGE_ON_HR(hr, NAMEOF(d3d11Device->CreateSamplerState) + L" failed!");
}

void MeshRenderer::Initialize()
{
}

void MeshRenderer::Update()
{
}

void MeshRenderer::Draw()
{
    d3d11DevCon->VSSetShaderResources(1, 1, &bufferPositionsSRV);
    d3d11DevCon->VSSetShaderResources(2, 1, &bufferTextureCoordinatesSRV);
    d3d11DevCon->VSSetShaderResources(3, 1, &bufferNormalsSRV);

    constantBufferData.World = sceneObject->transform->worldMatrix.Transpose();
    constantBufferData.WorldInverseTranspose = constantBufferData.World.Invert().Transpose();
    constantBufferData.View = camera->GetViewMatrix().Transpose();
    constantBufferData.Projection = camera->GetProjectionMatrix().Transpose();
    constantBufferData.cameraPosition = camera->GetPosition();
    constantBufferData.shadingMode = (int)settings->shadingMode;

    d3d11DevCon->UpdateSubresource(constantBuffer, 0, NULL, &constantBufferData, 0, 0);
    d3d11DevCon->VSSetConstantBuffers(0, 1, &constantBuffer);
    d3d11DevCon->PSSetConstantBuffers(0, 1, &constantBuffer);

    d3d11DevCon->IASetInputLayout(meshShader->inputLayout);
    d3d11DevCon->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    d3d11DevCon->VSSetShader(meshShader->vertexShader, NULL, 0);
    d3d11DevCon->GSSetShader(NULL, NULL, 0);
    d3d11DevCon->PSSetShader(meshShader->pixelShader, NULL, 0);
    d3d11DevCon->PSSetSamplers(0, 1, &samplerState);

    UINT vertexCount = 0;

    for (int i = 0; i < triangles.size(); i++)
    {
        d3d11DevCon->PSSetShaderResources(0, 1, &textureSRVs[i]);

        UINT offset = 0;
        UINT stride = sizeof(MeshVertex);
        d3d11DevCon->IASetVertexBuffers(0, 1, &triangles[i], &stride, &offset);

        d3d11DevCon->Draw(triangleVertexCounts[i], 0);

        vertexCount += triangleVertexCounts[i];
    }

    GUI::vertexCount = vertexCount;

    // After drawing, store the previous matrices for optical flow computation
    constantBufferData.PreviousWorld = constantBufferData.World;
    constantBufferData.PreviousView = constantBufferData.View;
    constantBufferData.PreviousProjection = constantBufferData.Projection;
    constantBufferData.PreviousWorldInverseTranspose = constantBufferData.WorldInverseTranspose;
}

void MeshRenderer::Release()
{
    SAFE_RELEASE(bufferPositions);
    SAFE_RELEASE(bufferPositionsSRV);
    SAFE_RELEASE(bufferTextureCoordinates);
    SAFE_RELEASE(bufferTextureCoordinatesSRV);
    SAFE_RELEASE(bufferNormals);
    SAFE_RELEASE(bufferNormalsSRV);
    SAFE_RELEASE(constantBuffer);
    SAFE_RELEASE(samplerState);

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
    triangleVertexCounts.clear();
}
