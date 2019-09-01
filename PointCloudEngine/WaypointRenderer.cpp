#include "PrecompiledHeader.h"
#include "WaypointRenderer.h"

void PointCloudEngine::WaypointRenderer::Initialize()
{
	// Create the constant buffer
	D3D11_BUFFER_DESC constantBufferDesc;
	ZeroMemory(&constantBufferDesc, sizeof(constantBufferDesc));
	constantBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	constantBufferDesc.ByteWidth = sizeof(WaypointRendererConstantBuffer);
	constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	hr = d3d11Device->CreateBuffer(&constantBufferDesc, NULL, &constantBuffer);
	ERROR_MESSAGE_ON_FAIL(hr, NAMEOF(d3d11Device->CreateBuffer) + L" failed for the " + NAMEOF(constantBuffer));
}

void PointCloudEngine::WaypointRenderer::Update()
{
	if (Input::GetKeyDown(Keyboard::Insert))
	{
		waypointPositions.push_back(camera->GetPosition());
		waypointForwards.push_back(camera->GetForward());
		UpdateVertexBuffer();
	}

	if (Input::GetKeyDown(Keyboard::Delete) && waypointPositions.size() > 0)
	{
		waypointPositions.pop_back();
		waypointForwards.pop_back();
		UpdateVertexBuffer();
	}
}

void PointCloudEngine::WaypointRenderer::Draw()
{
	// Set shader and input layout
	d3d11DevCon->VSSetShader(waypointShader->vertexShader, 0, 0);
	d3d11DevCon->PSSetShader(waypointShader->pixelShader, 0, 0);
	d3d11DevCon->IASetInputLayout(waypointShader->inputLayout);

	// Bind the vertex buffer and index buffer to the input assembler (IA)
	UINT offset = 0;
	UINT stride = sizeof(Vector3);
	d3d11DevCon->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);

	// Render as a line list
	d3d11DevCon->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);

	// Set shader constant buffer variables
	constantBufferData.View = camera->GetViewMatrix().Transpose();
	constantBufferData.Projection = camera->GetProjectionMatrix().Transpose();

	// Update constant buffer and draw
	d3d11DevCon->UpdateSubresource(constantBuffer, 0, NULL, &constantBufferData, 0, 0);
	d3d11DevCon->VSSetConstantBuffers(0, 1, &constantBuffer);
	d3d11DevCon->Draw(vertices.size(), 0);
}

void PointCloudEngine::WaypointRenderer::Release()
{
	SAFE_RELEASE(vertexBuffer);
	SAFE_RELEASE(constantBuffer);
}

void PointCloudEngine::WaypointRenderer::UpdateVertexBuffer()
{
	SAFE_RELEASE(vertexBuffer);

	if (waypointPositions.size() > 0)
	{
		vertices.clear();

		// Create a connected line list from the waypoint data
		for (UINT i = 0; i < waypointPositions.size(); i++)
		{
			vertices.push_back(waypointPositions[i]);
			vertices.push_back(waypointPositions[i] + (1.0f / settings->scale) * waypointForwards[i]);
			vertices.push_back(waypointPositions[i]);
		}

		// Create a vertex buffer description
		D3D11_BUFFER_DESC vertexBufferDesc;
		ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));
		vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
		vertexBufferDesc.ByteWidth = sizeof(Vector3) * vertices.size();
		vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

		// Fill a D3D11_SUBRESOURCE_DATA struct with the data we want in the buffer
		D3D11_SUBRESOURCE_DATA vertexBufferData;
		ZeroMemory(&vertexBufferData, sizeof(vertexBufferData));
		vertexBufferData.pSysMem = &vertices[0];

		// Create the vertex buffer
		hr = d3d11Device->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &vertexBuffer);
		ERROR_MESSAGE_ON_FAIL(hr, NAMEOF(d3d11Device->CreateBuffer) + L" failed for the " + NAMEOF(vertexBuffer));
	}
}
