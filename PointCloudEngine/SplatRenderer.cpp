#include "SplatRenderer.h"

SplatRenderer::SplatRenderer(const std::wstring &plyfile)
{
    // Try to load the file
    if (!LoadPlyFile(vertices, plyfile))
    {
        throw std::exception("Could not load .ply file!");
    }

    // Set the default values
    constantBufferData.fovAngleY = settings->fovAngleY;
}

void SplatRenderer::Initialize()
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
	ERROR_MESSAGE_ON_FAIL(hr, NAMEOF(d3d11Device->CreateBuffer) + L" failed for the " + NAMEOF(vertexBuffer));

    // Create the constant buffer for WVP
    D3D11_BUFFER_DESC cbDescWVP;
    ZeroMemory(&cbDescWVP, sizeof(cbDescWVP));
    cbDescWVP.Usage = D3D11_USAGE_DEFAULT;
    cbDescWVP.ByteWidth = sizeof(SplatRendererConstantBuffer);
    cbDescWVP.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDescWVP.CPUAccessFlags = 0;
    cbDescWVP.MiscFlags = 0;

    hr = d3d11Device->CreateBuffer(&cbDescWVP, NULL, &constantBuffer);
	ERROR_MESSAGE_ON_FAIL(hr, NAMEOF(d3d11Device->CreateBuffer) + L" failed for the " + NAMEOF(constantBuffer));
}

void SplatRenderer::Update()
{
    
}

void SplatRenderer::Draw()
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
    constantBufferData.View = camera->GetViewMatrix().Transpose();
    constantBufferData.Projection = camera->GetProjectionMatrix().Transpose();
    constantBufferData.cameraPosition = camera->GetPosition();
	constantBufferData.samplingRate = settings->samplingRate;

    // Update effect file buffer, set shader buffer to our created buffer
    d3d11DevCon->UpdateSubresource(constantBuffer, 0, NULL, &constantBufferData, 0, 0);
    d3d11DevCon->VSSetConstantBuffers(0, 1, &constantBuffer);
    d3d11DevCon->GSSetConstantBuffers(0, 1, &constantBuffer);

    d3d11DevCon->Draw(vertices.size(), 0);
}

void SplatRenderer::Release()
{
    SAFE_RELEASE(vertexBuffer);
    SAFE_RELEASE(constantBuffer);
}

void PointCloudEngine::SplatRenderer::SetLighting(const bool& useLighting)
{
	// Nothing to do here
}

void PointCloudEngine::SplatRenderer::GetBoundingCubePositionAndSize(Vector3 &outPosition, float &outSize)
{
    outPosition = Vector3::Zero;
    outSize = settings->scale;
}

void PointCloudEngine::SplatRenderer::RemoveComponentFromSceneObject()
{
	sceneObject->RemoveComponent(this);
}
