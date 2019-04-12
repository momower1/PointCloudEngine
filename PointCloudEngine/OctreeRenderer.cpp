#include "OctreeRenderer.h"

OctreeRenderer::OctreeRenderer(const std::wstring &plyfile)
{
    // Create the octree, throws exception on fail
    octree = new Octree(plyfile);

    // Text for showing properties
    text = Hierarchy::Create(L"OctreeRendererText");
    textRenderer = text->AddComponent(new TextRenderer(TextRenderer::GetSpriteFont(L"Consolas"), false));

    text->transform->position = Vector3(-1, -0.85f, 0);
    text->transform->scale = 0.35f * Vector3::One;

    // Initialize constant buffer data
    constantBufferData.fovAngleY = settings->fovAngleY;
    constantBufferData.splatSize = 0.01f;
}

void OctreeRenderer::Initialize(SceneObject *sceneObject)
{
    // Create the constant buffer for WVP
    D3D11_BUFFER_DESC cbDescWVP;
    ZeroMemory(&cbDescWVP, sizeof(cbDescWVP));
    cbDescWVP.Usage = D3D11_USAGE_DEFAULT;
    cbDescWVP.ByteWidth = sizeof(OctreeRendererConstantBuffer);
    cbDescWVP.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDescWVP.CPUAccessFlags = 0;
    cbDescWVP.MiscFlags = 0;

    hr = d3d11Device->CreateBuffer(&cbDescWVP, NULL, &constantBuffer);
    ErrorMessage(L"CreateBuffer failed for the constant buffer matrices.", L"Initialize", __FILEW__, __LINE__, hr);
}

void OctreeRenderer::Update(SceneObject *sceneObject)
{
    // Select octree level with arrow keys (level -1 means that the level will be ignored)
    if (Input::GetKeyDown(Keyboard::Left) && (level > -1))
    {
        level--;
    }
    else if (Input::GetKeyDown(Keyboard::Right) && ((octreeVertices.size() > 0) || (level < 0)))
    {
        level++;
    }

    // Toggle draw splats
    if (Input::GetKeyDown(Keyboard::Enter))
    {
        viewMode = (viewMode + 1) % 3;
    }

    // Set the text
    int splatSizePixels = settings->resolutionY * constantBufferData.splatSize * constantBufferData.overlapFactor;
    textRenderer->text = L"Splat Size: " + std::to_wstring(splatSizePixels) + L" Pixel\n";

    if (viewMode == 0)
    {
        textRenderer->text.append(L"Node View Mode: Splats\n");
    }
    else if (viewMode == 1)
    {
        textRenderer->text.append(L"Node View Mode: Bounding Cubes\n");
    }
    else if (viewMode == 2)
    {
        textRenderer->text.append(L"Node View Mode: Normal Clusters\n");
    }

    textRenderer->text.append(L"Octree Level: ");
    textRenderer->text.append((level < 0) ? L"AUTO" : std::to_wstring(level));
    textRenderer->text.append(L", Vertex Count: " + std::to_wstring(octreeVertices.size()));
}

void OctreeRenderer::Draw(SceneObject *sceneObject)
{
    if (useComputeShader)
    {
        DrawOctreeCompute(sceneObject);
    }
    else
    {
        DrawOctree(sceneObject);
    }
}

void OctreeRenderer::Release()
{
    SafeDelete(octree);

    Hierarchy::ReleaseSceneObject(text);

    SAFE_RELEASE(vertexBuffer);
    SAFE_RELEASE(constantBuffer);
}

void PointCloudEngine::OctreeRenderer::SetSplatSize(const float &splatSize)
{
    constantBufferData.splatSize = splatSize;
}

void PointCloudEngine::OctreeRenderer::GetBoundingCubePositionAndSize(Vector3 &outPosition, float &outSize)
{
    octree->GetRootPositionAndSize(outPosition, outSize);
}

void PointCloudEngine::OctreeRenderer::DrawOctree(SceneObject *sceneObject)
{
    // Create new buffer from the current octree traversal on the cpu
    if (level < 0)
    {
        Matrix worldInverse = sceneObject->transform->worldMatrix.Invert();
        Vector3 cameraPosition = camera->GetPosition();
        Vector3 localCameraPosition = Vector4::Transform(Vector4(cameraPosition.x, cameraPosition.y, cameraPosition.z, 1), worldInverse);

        octreeVertices = octree->GetVertices(localCameraPosition, constantBufferData.splatSize);
    }
    else
    {
        octreeVertices = octree->GetVerticesAtLevel(level);
    }

    int octreeVerticesSize = octreeVertices.size();

    if (octreeVerticesSize > 0)
    {
        // Release vertex buffer
        SAFE_RELEASE(vertexBuffer);

        // Create a vertex buffer description with dynamic write access
        D3D11_BUFFER_DESC vertexBufferDesc;
        ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));
        vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
        vertexBufferDesc.ByteWidth = sizeof(OctreeNodeVertex) * octreeVerticesSize;
        vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        vertexBufferDesc.CPUAccessFlags = 0;

        // Fill a D3D11_SUBRESOURCE_DATA struct with the data we want in the buffer
        D3D11_SUBRESOURCE_DATA vertexBufferData;
        ZeroMemory(&vertexBufferData, sizeof(vertexBufferData));
        vertexBufferData.pSysMem = &octreeVertices[0];

        // Create the buffer
        hr = d3d11Device->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &vertexBuffer);
        ErrorMessage(L"CreateBuffer failed for the vertex buffer.", L"Initialize", __FILEW__, __LINE__, hr);

        vertexBufferSize = octreeVerticesSize;

        // Set the shaders
        if (viewMode == 0)
        {
            d3d11DevCon->VSSetShader(octreeSplatShader->vertexShader, 0, 0);
            d3d11DevCon->GSSetShader(octreeSplatShader->geometryShader, 0, 0);
            d3d11DevCon->PSSetShader(octreeSplatShader->pixelShader, 0, 0);
        }
        else if (viewMode == 1)
        {
            d3d11DevCon->VSSetShader(octreeCubeShader->vertexShader, 0, 0);
            d3d11DevCon->GSSetShader(octreeCubeShader->geometryShader, 0, 0);
            d3d11DevCon->PSSetShader(octreeCubeShader->pixelShader, 0, 0);
        }
        else if (viewMode == 2)
        {
            d3d11DevCon->VSSetShader(octreeClusterShader->vertexShader, 0, 0);
            d3d11DevCon->GSSetShader(octreeClusterShader->geometryShader, 0, 0);
            d3d11DevCon->PSSetShader(octreeClusterShader->pixelShader, 0, 0);
        }

        // Set the Input (Vertex) Layout
        d3d11DevCon->IASetInputLayout(octreeCubeShader->inputLayout);

        // Bind the vertex buffer and index buffer to the input assembler (IA)
        UINT offset = 0;
        UINT stride = sizeof(OctreeNodeVertex);
        d3d11DevCon->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);

        // Set primitive topology
        d3d11DevCon->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

        // Set shader constant buffer variables
        constantBufferData.World = sceneObject->transform->worldMatrix.Transpose();
        constantBufferData.WorldInverseTranspose = constantBufferData.World.Invert().Transpose();
        constantBufferData.View = camera->GetViewMatrix().Transpose();
        constantBufferData.Projection = camera->GetProjectionMatrix().Transpose();
        constantBufferData.cameraPosition = camera->GetPosition();

        // Draw overlapping splats to make sure that continuous surfaces are drawn
        // Higher overlap factor reduces the spacing between tilted splats but reduces the detail (blend overlapping splats to improve this)
        // 1.0f = Orthogonal splats to the camera are as large as the pixel area they should fill and do not overlap
        // 2.0f = Orthogonal splats to the camera are twice as large and overlap with all their surrounding splats
        constantBufferData.overlapFactor = 1.75f;

        // Update effect file buffer, set shader buffer to our created buffer
        d3d11DevCon->UpdateSubresource(constantBuffer, 0, NULL, &constantBufferData, 0, 0);
        d3d11DevCon->VSSetConstantBuffers(0, 1, &constantBuffer);
        d3d11DevCon->GSSetConstantBuffers(0, 1, &constantBuffer);

        d3d11DevCon->Draw(octreeVerticesSize, 0);
    }
}

void PointCloudEngine::OctreeRenderer::DrawOctreeCompute(SceneObject *sceneObject)
{
    // TODO: Compile compute shader, initialize buffers
    if (computeShader == NULL)
    {
        // Compile compute shader
    }


    // TODO: Use compute shader to traverse the octree
    // Store all the vertices that should be drawn in an appendstructuredbuffer and draw them by index
}
