#include "GroundTruthRenderer.h"

GroundTruthRenderer::GroundTruthRenderer(const std::wstring &plyfile)
{
    // Try to load the file
    if (!LoadPlyFile(vertices, plyfile))
    {
        throw std::exception("Could not load .ply file!");
    }

    // Set the default values
    constantBufferData.fovAngleY = settings->fovAngleY;
}

void GroundTruthRenderer::Initialize()
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
    cbDescWVP.ByteWidth = sizeof(GroundTruthRendererConstantBuffer);
    cbDescWVP.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDescWVP.CPUAccessFlags = 0;
    cbDescWVP.MiscFlags = 0;

    hr = d3d11Device->CreateBuffer(&cbDescWVP, NULL, &constantBuffer);
	ERROR_MESSAGE_ON_FAIL(hr, NAMEOF(d3d11Device->CreateBuffer) + L" failed for the " + NAMEOF(constantBuffer));
}

void GroundTruthRenderer::Update()
{
    
}

void GroundTruthRenderer::Draw()
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

void GroundTruthRenderer::Release()
{
    SAFE_RELEASE(vertexBuffer);
    SAFE_RELEASE(constantBuffer);
}

void PointCloudEngine::GroundTruthRenderer::SetLighting(const bool& useLighting)
{
	// Nothing to do here
}

void PointCloudEngine::GroundTruthRenderer::GetBoundingCubePositionAndSize(Vector3 &outPosition, float &outSize)
{
	// Calculate center and size of the bounding cube that fully encloses the point cloud
	Vector3 minPosition = vertices.front().position;
	Vector3 maxPosition = minPosition;

	for (auto it = vertices.begin(); it != vertices.end(); it++)
	{
		Vertex v = *it;

		minPosition = Vector3::Min(minPosition, v.position);
		maxPosition = Vector3::Max(maxPosition, v.position);
	}

	// Compute the root node position and size, will be used to compute all the other node positions and sizes at runtime
	Vector3 diagonal = maxPosition - minPosition;

    outPosition = minPosition + 0.5f * diagonal;
    outSize = max(max(diagonal.x, diagonal.y), diagonal.z);
}

void PointCloudEngine::GroundTruthRenderer::RemoveComponentFromSceneObject()
{
	sceneObject->RemoveComponent(this);
}
