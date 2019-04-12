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
    octreeRendererConstantBufferData.fovAngleY = settings->fovAngleY;
    octreeRendererConstantBufferData.splatSize = 0.01f;
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

    hr = d3d11Device->CreateBuffer(&cbDescWVP, NULL, &octreeRendererConstantBuffer);
    ErrorMessage(L"CreateBuffer failed for the constant buffer matrices.", L"Initialize", __FILEW__, __LINE__, hr);

    // Create the constant buffer for the compute shader
    D3D11_BUFFER_DESC computeShaderConstantBufferDesc;
    ZeroMemory(&computeShaderConstantBufferDesc, sizeof(computeShaderConstantBufferDesc));
    computeShaderConstantBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    computeShaderConstantBufferDesc.ByteWidth = sizeof(ComputeShaderConstantBuffer);
    computeShaderConstantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    computeShaderConstantBufferDesc.CPUAccessFlags = 0;
    computeShaderConstantBufferDesc.MiscFlags = 0;

    hr = d3d11Device->CreateBuffer(&computeShaderConstantBufferDesc, NULL, &computeShaderConstantBuffer);
    ErrorMessage(L"CreateBuffer failed for the compute shader constant buffer.", L"Initialize", __FILEW__, __LINE__, hr);
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
    int splatSizePixels = settings->resolutionY * octreeRendererConstantBufferData.splatSize * octreeRendererConstantBufferData.overlapFactor;
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
    // Transform the camera position into local space and save it in the constant buffers
    Matrix worldInverse = sceneObject->transform->worldMatrix.Invert();
    Vector3 cameraPosition = camera->GetPosition();
    Vector3 localCameraPosition = Vector4::Transform(Vector4(cameraPosition.x, cameraPosition.y, cameraPosition.z, 1), worldInverse);

    // Set shader constant buffer variables
    octreeRendererConstantBufferData.World = sceneObject->transform->worldMatrix.Transpose();
    octreeRendererConstantBufferData.WorldInverseTranspose = octreeRendererConstantBufferData.World.Invert().Transpose();
    octreeRendererConstantBufferData.View = camera->GetViewMatrix().Transpose();
    octreeRendererConstantBufferData.Projection = camera->GetProjectionMatrix().Transpose();
    octreeRendererConstantBufferData.cameraPosition = camera->GetPosition();

    // Draw overlapping splats to make sure that continuous surfaces are drawn
    // Higher overlap factor reduces the spacing between tilted splats but reduces the detail (blend overlapping splats to improve this)
    // 1.0f = Orthogonal splats to the camera are as large as the pixel area they should fill and do not overlap
    // 2.0f = Orthogonal splats to the camera are twice as large and overlap with all their surrounding splats
    octreeRendererConstantBufferData.overlapFactor = 1.75f;

    // Update the hlsl file buffer, set shader buffer to our created buffer
    d3d11DevCon->UpdateSubresource(octreeRendererConstantBuffer, 0, NULL, &octreeRendererConstantBufferData, 0, 0);

    // Set shader buffer
    d3d11DevCon->VSSetConstantBuffers(0, 1, &octreeRendererConstantBuffer);
    d3d11DevCon->GSSetConstantBuffers(0, 1, &octreeRendererConstantBuffer);

    if (useComputeShader)
    {
        DrawOctreeCompute(sceneObject, localCameraPosition);
    }
    else
    {
        DrawOctree(sceneObject, localCameraPosition);
    }
}

void OctreeRenderer::Release()
{
    SafeDelete(octree);

    Hierarchy::ReleaseSceneObject(text);

    SAFE_RELEASE(octreeRendererConstantBuffer);
    SAFE_RELEASE(computeShaderConstantBuffer);
}

void PointCloudEngine::OctreeRenderer::SetSplatSize(const float &splatSize)
{
    octreeRendererConstantBufferData.splatSize = splatSize;
    computeShaderConstantBufferData.splatSize = splatSize;
}

void PointCloudEngine::OctreeRenderer::GetBoundingCubePositionAndSize(Vector3 &outPosition, float &outSize)
{
    octree->GetRootPositionAndSize(outPosition, outSize);
}

void PointCloudEngine::OctreeRenderer::DrawOctree(SceneObject *sceneObject, const Vector3 &localCameraPosition)
{
    // Create new buffer from the current octree traversal on the cpu
    if (level < 0)
    {
        octreeVertices = octree->GetVertices(localCameraPosition, octreeRendererConstantBufferData.splatSize);
    }
    else
    {
        octreeVertices = octree->GetVerticesAtLevel(level);
    }

    int octreeVerticesSize = octreeVertices.size();

    if (octreeVerticesSize > 0)
    {
        // Create a new vertex buffer
        ID3D11Buffer* vertexBuffer = NULL;

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

        d3d11DevCon->Draw(octreeVerticesSize, 0);

        SAFE_RELEASE(vertexBuffer);
    }
}

void PointCloudEngine::OctreeRenderer::DrawOctreeCompute(SceneObject *sceneObject, const Vector3 &localCameraPosition)
{
    // Set constant buffer data
    computeShaderConstantBufferData.localCameraPosition = localCameraPosition;

    // Update constant buffer
    d3d11DevCon->UpdateSubresource(computeShaderConstantBuffer, 0, NULL, &computeShaderConstantBufferData, 0, 0);

    // Set the constant buffer
    d3d11DevCon->CSSetConstantBuffers(0, 1, &computeShaderConstantBuffer);

    // Create vertex append buffer
    D3D11_BUFFER_DESC vertexBufferDesc;
    vertexBufferDesc.ByteWidth = sizeof(OctreeNode);
    vertexBufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
    vertexBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    vertexBufferDesc.StructureByteStride = sizeof(OctreeNode);
    vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    vertexBufferDesc.CPUAccessFlags = 0;

    // Create root append buffer
    D3D11_BUFFER_DESC rootBufferDesc;
    rootBufferDesc.ByteWidth = sizeof(OctreeNode);
    rootBufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
    rootBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    rootBufferDesc.StructureByteStride = sizeof(OctreeNode);
    rootBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    rootBufferDesc.CPUAccessFlags = 0;

    // TODO: Use compute shader to traverse the octree
    d3d11DevCon->CSSetShader(octreeComputeShader->computeShader, 0, 0);

    // Store all the vertices that should be drawn in an appendstructuredbuffer and draw them by index

    // TODO: Set input layout, ..., Draw(...)
}
