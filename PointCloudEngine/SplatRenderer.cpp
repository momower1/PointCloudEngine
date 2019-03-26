#include "SplatRenderer.h"

SplatRenderer::SplatRenderer(std::wstring plyfile)
{
    vertices = LoadPlyFile(plyfile);

    // Set the default values
    constantBufferData.splatSize = 0.01f;
    constantBufferData.fovAngleY = fovAngleY;
}

void SplatRenderer::Initialize(SceneObject *sceneObject)
{
    // Create a vertex buffer description
    D3D11_BUFFER_DESC vertexBufferDesc;
    ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));
    vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    vertexBufferDesc.ByteWidth = sizeof(Vertex) * vertices.size();
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexBufferDesc.CPUAccessFlags = 0;
    vertexBufferDesc.MiscFlags = 0;

    // Fill a D3D11_SUBRESOURCE_DATA struct with the data we want in the buffer
    D3D11_SUBRESOURCE_DATA vertexBufferData;
    ZeroMemory(&vertexBufferData, sizeof(vertexBufferData));
    vertexBufferData.pSysMem = &vertices[0];

    // Create the buffer
    hr = d3d11Device->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &vertexBuffer);
    ErrorMessage(L"CreateBuffer failed for the vertex buffer.", L"Initialize", __FILEW__, __LINE__, hr);

    // Create the constant buffer for WVP
    D3D11_BUFFER_DESC cbDescWVP;
    ZeroMemory(&cbDescWVP, sizeof(cbDescWVP));
    cbDescWVP.Usage = D3D11_USAGE_DEFAULT;
    cbDescWVP.ByteWidth = sizeof(SplatRendererConstantBuffer);
    cbDescWVP.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDescWVP.CPUAccessFlags = 0;
    cbDescWVP.MiscFlags = 0;

    hr = d3d11Device->CreateBuffer(&cbDescWVP, NULL, &constantBuffer);
    ErrorMessage(L"CreateBuffer failed for the constant buffer matrices.", L"Initialize", __FILEW__, __LINE__, hr);
}

void SplatRenderer::Update(SceneObject *sceneObject)
{
    
}

void SplatRenderer::Draw(SceneObject *sceneObject)
{
    // Set the shaders
    d3d11DevCon->VSSetShader(splatShader->vertexShader, 0, 0);
    d3d11DevCon->GSSetShader(splatShader->geometryShader, 0, 0);
    d3d11DevCon->PSSetShader(splatShader->pixelShader, 0, 0);

    // Set the Input (Vertex) Layout
    d3d11DevCon->IASetInputLayout(splatShader->inputLayout);

    // Bind the vertex buffer and index buffer to the input assembler (IA)
    UINT offset = 0;
    UINT stride = sizeof(Vertex);
    d3d11DevCon->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);

    // Set primitive topology
    d3d11DevCon->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

    // Set shader constant buffer variables
    constantBufferData.World = sceneObject->transform->worldMatrix.Transpose();
    constantBufferData.WorldInverseTranspose = constantBufferData.World.Invert().Transpose();
    constantBufferData.View = camera.view.Transpose();
    constantBufferData.Projection = camera.projection.Transpose();
    constantBufferData.cameraPosition = camera.position;

    // Update effect file buffer, set shader buffer to our created buffer
    d3d11DevCon->UpdateSubresource(constantBuffer, 0, NULL, &constantBufferData, 0, 0);
    d3d11DevCon->VSSetConstantBuffers(0, 1, &constantBuffer);
    d3d11DevCon->GSSetConstantBuffers(0, 1, &constantBuffer);

    d3d11DevCon->Draw(vertices.size(), 0);
}

void SplatRenderer::Release()
{
    SafeRelease(vertexBuffer);
    SafeRelease(constantBuffer);
}

void PointCloudEngine::SplatRenderer::SetSplatSize(const float &splatSize)
{
    constantBufferData.splatSize = splatSize;
}
