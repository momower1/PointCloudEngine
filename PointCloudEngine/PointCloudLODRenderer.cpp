#include "PointCloudLODRenderer.h"

std::vector<PointCloudLODRenderer*> PointCloudLODRenderer::sharedPointCloudLODRenderers;

PointCloudLODRenderer* PointCloudLODRenderer::CreateShared(std::wstring plyfile)
{
    PointCloudLODRenderer *pointCloudLODRenderer = new PointCloudLODRenderer(plyfile);
    pointCloudLODRenderer->shared = true;

    sharedPointCloudLODRenderers.push_back(pointCloudLODRenderer);
    return pointCloudLODRenderer;
}

void PointCloudLODRenderer::ReleaseAllSharedPointCloudLODRenderers()
{
    for (auto it = sharedPointCloudLODRenderers.begin(); it != sharedPointCloudLODRenderers.end(); it++)
    {
        PointCloudLODRenderer *pointCloudLODRenderer = *it;
        pointCloudLODRenderer->Release();
        delete pointCloudLODRenderer;
    }

    sharedPointCloudLODRenderers.clear();
}

PointCloudLODRenderer::PointCloudLODRenderer(std::wstring plyfile)
{
    vertices = LoadPlyFile(plyfile);

    // Set the default radius
    pointCloudLODConstantBufferData.radius = 0.02f;
}

void PointCloudLODRenderer::Initialize(SceneObject *sceneObject)
{
    // Create a vertex buffer description
    D3D11_BUFFER_DESC vertexBufferDesc;
    ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));
    vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    vertexBufferDesc.ByteWidth = sizeof(PointCloudVertex) * vertices.size();
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
    cbDescWVP.ByteWidth = sizeof(PointCloudLODConstantBuffer);
    cbDescWVP.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDescWVP.CPUAccessFlags = 0;
    cbDescWVP.MiscFlags = 0;

    hr = d3d11Device->CreateBuffer(&cbDescWVP, NULL, &pointCloudLODConstantBuffer);
    ErrorMessage(L"CreateBuffer failed for the constant buffer matrices.", L"Initialize", __FILEW__, __LINE__, hr);
}

void PointCloudLODRenderer::Update(SceneObject *sceneObject)
{
    if (Input::GetKey(Keyboard::Up))
    {
        pointCloudLODConstantBufferData.radius += dt * 0.01f;
    }
    else if (Input::GetKey(Keyboard::Down))
    {
        pointCloudLODConstantBufferData.radius -= dt * 0.01f;
    }

    pointCloudLODConstantBufferData.radius = max(0.0002f, pointCloudLODConstantBufferData.radius);
}

void PointCloudLODRenderer::Draw(SceneObject *sceneObject)
{
    // Set the shaders
    d3d11DevCon->VSSetShader(pointCloudLODShader->vertexShader, 0, 0);
    d3d11DevCon->GSSetShader(pointCloudLODShader->geometryShader, 0, 0);
    d3d11DevCon->PSSetShader(pointCloudLODShader->pixelShader, 0, 0);

    // Set the Input (Vertex) Layout
    d3d11DevCon->IASetInputLayout(pointCloudShader->inputLayout);

    // Bind the vertex buffer and index buffer to the input assembler (IA)
    UINT offset = 0;
    UINT stride = sizeof(PointCloudVertex);
    d3d11DevCon->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);

    // Set primitive topology
    d3d11DevCon->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

    // Set shader constant buffer variables
    pointCloudLODConstantBufferData.World = sceneObject->transform->worldMatrix.Transpose();
    pointCloudLODConstantBufferData.WorldInverseTranspose = pointCloudLODConstantBufferData.World.Invert().Transpose();
    pointCloudLODConstantBufferData.View = camera.view.Transpose();
    pointCloudLODConstantBufferData.Projection = camera.projection.Transpose();

    // Update effect file buffer, set shader buffer to our created buffer
    d3d11DevCon->UpdateSubresource(pointCloudLODConstantBuffer, 0, NULL, &pointCloudLODConstantBufferData, 0, 0);
    d3d11DevCon->VSSetConstantBuffers(0, 1, &pointCloudLODConstantBuffer);
    d3d11DevCon->GSSetConstantBuffers(0, 1, &pointCloudLODConstantBuffer);

    d3d11DevCon->Draw(vertices.size(), 0);
}

void PointCloudLODRenderer::Release()
{
    SafeRelease(vertexBuffer);
    SafeRelease(pointCloudLODConstantBuffer);
}
