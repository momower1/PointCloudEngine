#include "GroundTruthRenderer.h"

GroundTruthRenderer::GroundTruthRenderer(const std::wstring &pointcloudFile)
{
    // Try to load the file
    if (!LoadPointcloudFile(vertices, pointcloudFile))
    {
        throw std::exception("Could not load .pointcloud file!");
    }

	// Calculate center and size of the bounding cube that fully encloses the point cloud
	Vector3 minPosition = vertices.front().position;
	Vector3 maxPosition = minPosition;

	for (auto it = vertices.begin(); it != vertices.end(); it++)
	{
		Vertex v = *it;

		minPosition = Vector3::Min(minPosition, v.position);
		maxPosition = Vector3::Max(maxPosition, v.position);
	}

	Vector3 diagonal = maxPosition - minPosition;
	boundingCubePosition = minPosition + 0.5f * diagonal;
	boundingCubeSize = max(max(diagonal.x, diagonal.y), diagonal.z);

	// Create text renderer to display the controls
	helpTextRenderer = new TextRenderer(TextRenderer::GetSpriteFont(L"Consolas"), false);
	helpText = Hierarchy::Create(L"Help Text");
	helpText->AddComponent(helpTextRenderer);
	helpText->transform->position = Vector3(-1, 1, 0.5f);
	helpText->transform->scale = 0.35f * Vector3::One;

	// Text for showing properties
	textRenderer = new TextRenderer(TextRenderer::GetSpriteFont(L"Consolas"), false);
	text = Hierarchy::Create(L"GroundTruthRendererText");
	text->AddComponent(textRenderer);
	text->transform->position = Vector3(-1.0f, -0.635f, 0);
	text->transform->scale = 0.35f * Vector3::One;

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
    D3D11_BUFFER_DESC constantBufferDesc;
    ZeroMemory(&constantBufferDesc, sizeof(constantBufferDesc));
	constantBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	constantBufferDesc.ByteWidth = sizeof(GroundTruthRendererConstantBuffer);
	constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	constantBufferDesc.CPUAccessFlags = 0;
	constantBufferDesc.MiscFlags = 0;

    hr = d3d11Device->CreateBuffer(&constantBufferDesc, NULL, &constantBuffer);
	ERROR_MESSAGE_ON_FAIL(hr, NAMEOF(d3d11Device->CreateBuffer) + L" failed for the " + NAMEOF(constantBuffer));
}

void GroundTruthRenderer::Update()
{
	// Select density of the point cloud with arrow keys
	if (Input::GetKey(Keyboard::Right))
	{
		settings->density = min(1.0f, settings->density + 0.25f * dt);
	}
	else if (Input::GetKey(Keyboard::Left))
	{
		settings->density = max(0, settings->density - 0.25f * dt);
	}

	helpTextRenderer->text = L"[H] Toggle help\n";

	// Show help / controls
	if (settings->help)
	{
		helpTextRenderer->text.append(L"[O] Open .pointcloud file with (x,y,z,nx,ny,nz,red,green,blue) format\n");
		helpTextRenderer->text.append(L"[R] Switch to octree renderer\n");
		helpTextRenderer->text.append(L"[E/Q] Increase/decrease sampling rate\n");
		helpTextRenderer->text.append(L"[N/V] Increase/decrease blend factor\n");
		helpTextRenderer->text.append(L"[SHIFT] Increase WASD and Q/E input speed\n");
		helpTextRenderer->text.append(L"[RIGHT/LEFT] Increase/decrease point cloud density\n");
		helpTextRenderer->text.append(L"[ENTER] Switch view mode\n");
		helpTextRenderer->text.append(L"[SPACE] Rotate around y axis\n");
		helpTextRenderer->text.append(L"[F1-F6] Select camera position\n");
		helpTextRenderer->text.append(L"[MOUSE WHEEL] Scale\n");
		helpTextRenderer->text.append(L"[MOUSE] Rotate Camera\n");
		helpTextRenderer->text.append(L"[WASD] Move Camera\n");
		helpTextRenderer->text.append(L"[L] Toggle Lighting\n");
		helpTextRenderer->text.append(L"[B] Toggle Blending\n");
		helpTextRenderer->text.append(L"[F9] Screenshot\n");
		helpTextRenderer->text.append(L"[ESC] Quit\n");
	}

	// Set the text
	textRenderer->text = std::wstring(L"View Mode: TODO\n");
	textRenderer->text.append(L"Sampling Rate: " + std::to_wstring(settings->samplingRate) + L"\n");
	textRenderer->text.append(L"Blend Factor: " + std::to_wstring(settings->blendFactor) + L"\n");
	textRenderer->text.append(L"Point Density: " + std::to_wstring((UINT)(settings->density * 100)) + L"%\n");
	textRenderer->text.append(L"Blending " + std::wstring(settings->useBlending ? L"On, " : L"Off, "));
	textRenderer->text.append(L"Lighting " + std::wstring(settings->useLighting ? L"On\n" : L"Off\n"));
	textRenderer->text.append(L"Vertex Count: " + std::to_wstring((UINT)(vertices.size() * settings->density)) + L"\n");
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
	constantBufferData.View = camera->GetViewMatrix().Transpose();
	constantBufferData.Projection = camera->GetProjectionMatrix().Transpose();
    constantBufferData.WorldInverseTranspose = constantBufferData.World.Invert().Transpose();
	constantBufferData.WorldViewProjectionInverse = (sceneObject->transform->worldMatrix * camera->GetViewMatrix() * camera->GetProjectionMatrix()).Invert().Transpose();
    constantBufferData.cameraPosition = camera->GetPosition();
	constantBufferData.samplingRate = settings->samplingRate;
	constantBufferData.blendFactor = settings->blendFactor;
	constantBufferData.useBlending = false;

    // Update effect file buffer, set shader buffer to our created buffer
    d3d11DevCon->UpdateSubresource(constantBuffer, 0, NULL, &constantBufferData, 0, 0);
	d3d11DevCon->VSSetConstantBuffers(0, 1, &constantBuffer);
	d3d11DevCon->GSSetConstantBuffers(0, 1, &constantBuffer);
	d3d11DevCon->PSSetConstantBuffers(0, 1, &constantBuffer);

	// Only draw a portion of the point cloud to simulate the selected density
	// This requires the vertex indices to be distributed randomly (pointcloud files provide this feature)
	UINT vertexCount = vertices.size() * settings->density;

	if (settings->useBlending)
	{
		DrawBlended(vertexCount, constantBuffer, &constantBufferData, constantBufferData.useBlending);
	}
	else
	{
		d3d11DevCon->Draw(vertexCount, 0);
	}
}

void GroundTruthRenderer::Release()
{
	Hierarchy::ReleaseSceneObject(text);
	Hierarchy::ReleaseSceneObject(helpText);

    SAFE_RELEASE(vertexBuffer);
    SAFE_RELEASE(constantBuffer);
}

void PointCloudEngine::GroundTruthRenderer::GetBoundingCubePositionAndSize(Vector3 &outPosition, float &outSize)
{
	outPosition = boundingCubePosition;
	outSize = boundingCubeSize;
}

void PointCloudEngine::GroundTruthRenderer::RemoveComponentFromSceneObject()
{
	sceneObject->RemoveComponent(this);
}
