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

    // Create the buffer for the compute shader that stores all the octree nodes
    // Maximum size is ~4.2 GB (UINT MAX)
    D3D11_BUFFER_DESC nodesBufferDesc;
    ZeroMemory(&nodesBufferDesc, sizeof(nodesBufferDesc));
    nodesBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    nodesBufferDesc.ByteWidth = octree->nodes.size() * sizeof(OctreeNode);
    nodesBufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    nodesBufferDesc.StructureByteStride = sizeof(OctreeNode);
    nodesBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    nodesBufferDesc.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA nodesBufferData;
    ZeroMemory(&nodesBufferData, sizeof(nodesBufferData));
    nodesBufferData.pSysMem = octree->nodes.data();

    hr = d3d11Device->CreateBuffer(&nodesBufferDesc, &nodesBufferData, &nodesBuffer);
    ErrorMessage(L"CreateBuffer failed for the nodes buffer.", L"Initialize", __FILEW__, __LINE__, hr);

    D3D11_SHADER_RESOURCE_VIEW_DESC nodesBufferSRVDesc;
    ZeroMemory(&nodesBufferSRVDesc, sizeof(nodesBufferSRVDesc));
    nodesBufferSRVDesc.Format = DXGI_FORMAT_UNKNOWN;
    nodesBufferSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    nodesBufferSRVDesc.Buffer.ElementWidth = octree->nodes.size();

    hr = d3d11Device->CreateShaderResourceView(nodesBuffer, &nodesBufferSRVDesc, &nodesBufferSRV);
    ErrorMessage(L"CreateShaderResourceView failed for the nodes buffer srv.", L"Initialize", __FILEW__, __LINE__, hr);
}

void OctreeRenderer::Update(SceneObject *sceneObject)
{
    // Select octree level with arrow keys (level -1 means that the level will be ignored)
    if (Input::GetKeyDown(Keyboard::Left) && (level > -1))
    {
        level--;
    }
    else if (Input::GetKeyDown(Keyboard::Right) && ((vertexBufferCount > 0) || (level < 0)))
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
    textRenderer->text.append(L", Vertex Count: " + std::to_wstring(vertexBufferCount));
}

void OctreeRenderer::Draw(SceneObject *sceneObject)
{
    // Transform the camera position into local space and save it in the constant buffers
    Matrix world = sceneObject->transform->worldMatrix;
    Matrix worldInverse = world.Invert();
    Vector3 cameraPosition = camera->GetPosition();
    Vector3 localCameraPosition = Vector4::Transform(Vector4(cameraPosition.x, cameraPosition.y, cameraPosition.z, 1), worldInverse);

    // Set shader constant buffer variables
    octreeRendererConstantBufferData.World = world.Transpose();
    octreeRendererConstantBufferData.WorldInverseTranspose = worldInverse.Transpose();
    octreeRendererConstantBufferData.View = camera->GetViewMatrix().Transpose();
    octreeRendererConstantBufferData.Projection = camera->GetProjectionMatrix().Transpose();
    octreeRendererConstantBufferData.cameraPosition = cameraPosition;

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

    ID3D11Buffer *vertexBuffer = NULL;

    // Get the vertex buffer and use the specified implementation
    if (useComputeShader)
    {
        vertexBuffer = GetVertexBufferCompute(sceneObject, localCameraPosition);
    }
    else
    {
        vertexBuffer = GetVertexBuffer(sceneObject, localCameraPosition);
    }

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

    // Bind the vertex buffer and to the input assembler (IA)
    UINT offset = 0;
    UINT stride = sizeof(OctreeNodeVertex);
    d3d11DevCon->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);

    // Set primitive topology
    d3d11DevCon->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

    d3d11DevCon->Draw(vertexBufferCount, 0);

    SAFE_RELEASE(vertexBuffer);
}

void OctreeRenderer::Release()
{
    SafeDelete(octree);

    Hierarchy::ReleaseSceneObject(text);

    SAFE_RELEASE(nodesBuffer);
    SAFE_RELEASE(nodesBufferSRV);
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

ID3D11Buffer* PointCloudEngine::OctreeRenderer::GetVertexBuffer(SceneObject *sceneObject, const Vector3 &localCameraPosition)
{
    ID3D11Buffer* vertexBuffer = NULL;
    std::vector<OctreeNodeVertex> octreeVertices;

    // Create new buffer from the current octree traversal on the cpu
    if (level < 0)
    {
        octreeVertices = octree->GetVertices(localCameraPosition, octreeRendererConstantBufferData.splatSize);
    }
    else
    {
        octreeVertices = octree->GetVerticesAtLevel(level);
    }

    vertexBufferCount = octreeVertices.size();

    if (vertexBufferCount > 0)
    {
        // Create a vertex buffer description with dynamic write access
        D3D11_BUFFER_DESC vertexBufferDesc;
        ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));
        vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
        vertexBufferDesc.ByteWidth = sizeof(OctreeNodeVertex) * vertexBufferCount;
        vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        vertexBufferDesc.CPUAccessFlags = 0;

        // Fill a D3D11_SUBRESOURCE_DATA struct with the data we want in the buffer
        D3D11_SUBRESOURCE_DATA vertexBufferData;
        ZeroMemory(&vertexBufferData, sizeof(vertexBufferData));
        vertexBufferData.pSysMem = &octreeVertices[0];

        // Create the buffer
        hr = d3d11Device->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &vertexBuffer);
        ErrorMessage(L"CreateBuffer failed for the vertex buffer.", L"GetVertexBuffer", __FILEW__, __LINE__, hr);
    }

    return vertexBuffer;
}

ID3D11Buffer* PointCloudEngine::OctreeRenderer::GetVertexBufferCompute(SceneObject *sceneObject, const Vector3 &localCameraPosition)
{
    ID3D11Buffer *vertexBuffer = NULL;

    // Set constant buffer data
    computeShaderConstantBufferData.localCameraPosition = localCameraPosition;

    // Update constant buffer
    d3d11DevCon->UpdateSubresource(computeShaderConstantBuffer, 0, NULL, &computeShaderConstantBufferData, 0, 0);

    // Set the constant buffer
    d3d11DevCon->CSSetConstantBuffers(0, 1, &computeShaderConstantBuffer);

    // Create vertex append buffer, specify the maximum size of all the buffers here
    vertexBufferCount = 1000000;
    ID3D11Buffer *firstBuffer = NULL;
    ID3D11Buffer *secondBuffer = NULL;
    ID3D11Buffer *vertexAppendBuffer = NULL;
    ID3D11Buffer *structureCountBuffer = NULL;
    ID3D11UnorderedAccessView *firstBufferUAV = NULL;
    ID3D11UnorderedAccessView *secondBufferUAV = NULL;
    ID3D11UnorderedAccessView *vertexAppendBufferUAV = NULL;

    // Create general buffer description for append/consume buffer
    D3D11_BUFFER_DESC appendConsumeBufferDesc;
    ZeroMemory(&appendConsumeBufferDesc, sizeof(appendConsumeBufferDesc));
    appendConsumeBufferDesc.ByteWidth = vertexBufferCount * sizeof(int);
    appendConsumeBufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
    appendConsumeBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    appendConsumeBufferDesc.StructureByteStride = sizeof(int);
    appendConsumeBufferDesc.Usage = D3D11_USAGE_DEFAULT;

    // Create general UAV description for append/consume buffers
    D3D11_UNORDERED_ACCESS_VIEW_DESC appendConsumeBufferUAVDesc;
    ZeroMemory(&appendConsumeBufferUAVDesc, sizeof(appendConsumeBufferUAVDesc));
    appendConsumeBufferUAVDesc.Format = DXGI_FORMAT_UNKNOWN;
    appendConsumeBufferUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    appendConsumeBufferUAVDesc.Buffer.FirstElement = 0;
    appendConsumeBufferUAVDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_APPEND;
    appendConsumeBufferUAVDesc.Buffer.NumElements = vertexBufferCount;

    // Create the first buffer and its UAV
    hr = d3d11Device->CreateBuffer(&appendConsumeBufferDesc, NULL, &firstBuffer);
    ErrorMessage(L"CreateBuffer failed for first buffer.", L"GetVertexBufferCompute", __FILEW__, __LINE__, hr);

    hr = d3d11Device->CreateUnorderedAccessView(firstBuffer, &appendConsumeBufferUAVDesc, &firstBufferUAV);
    ErrorMessage(L"CreateUnorderedAccessView failed for first buffer uav.", L"GetVertexBufferCompute", __FILEW__, __LINE__, hr);

    // Create the second buffer and its UAV
    hr = d3d11Device->CreateBuffer(&appendConsumeBufferDesc, NULL, &secondBuffer);
    ErrorMessage(L"CreateBuffer failed for second buffer.", L"GetVertexBufferCompute", __FILEW__, __LINE__, hr);

    hr = d3d11Device->CreateUnorderedAccessView(secondBuffer, &appendConsumeBufferUAVDesc, &secondBufferUAV);
    ErrorMessage(L"CreateUnorderedAccessView failed for second buffer uav.", L"GetVertexBufferCompute", __FILEW__, __LINE__, hr);

    // Create the vertex append buffer
    D3D11_BUFFER_DESC vertexAppendBufferDesc;
    ZeroMemory(&vertexAppendBufferDesc, sizeof(vertexAppendBufferDesc));
    vertexAppendBufferDesc.ByteWidth = vertexBufferCount * sizeof(OctreeNodeVertex);
    vertexAppendBufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
    vertexAppendBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    vertexAppendBufferDesc.StructureByteStride = sizeof(OctreeNodeVertex);
    vertexAppendBufferDesc.Usage = D3D11_USAGE_DEFAULT;

    hr = d3d11Device->CreateBuffer(&vertexAppendBufferDesc, NULL, &vertexAppendBuffer);
    ErrorMessage(L"CreateBuffer failed for vertex buffer.", L"GetVertexBufferCompute", __FILEW__, __LINE__, hr);

    hr = d3d11Device->CreateUnorderedAccessView(vertexAppendBuffer, &appendConsumeBufferUAVDesc, &vertexAppendBufferUAV);
    ErrorMessage(L"CreateUnorderedAccessView failed for vertex buffer uav.", L"GetVertexBufferCompute", __FILEW__, __LINE__, hr);

    // Create the structure count buffer that is simply used to check for the size of the append/consume buffers
    D3D11_BUFFER_DESC structureCountBufferDesc;
    ZeroMemory(&structureCountBufferDesc, sizeof(structureCountBufferDesc));
    structureCountBufferDesc.ByteWidth = sizeof(int);
    structureCountBufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    structureCountBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    structureCountBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

    hr = d3d11Device->CreateBuffer(&structureCountBufferDesc, NULL, &structureCountBuffer);
    ErrorMessage(L"CreateBuffer failed for structure count buffer.", L"GetVertexBufferCompute", __FILEW__, __LINE__, hr);

    // Set 0 as the only entry (root index) for the first buffer, will be used as input consume buffer in the shader
    UINT data[4] = { 0, 0, 0, 0};
    d3d11DevCon->ClearUnorderedAccessViewUint(firstBufferUAV, data);

    // Use compute shader to traverse the octree
    UINT one = 1;
    UINT zero = 0;
    d3d11DevCon->CSSetShader(octreeComputeShader->computeShader, 0, 0);
    d3d11DevCon->CSSetShaderResources(0, 1, &nodesBufferSRV);
    d3d11DevCon->CSSetUnorderedAccessViews(0, 1, &firstBufferUAV, &one);
    d3d11DevCon->CSSetUnorderedAccessViews(1, 1, &secondBufferUAV, &zero);
    d3d11DevCon->CSSetUnorderedAccessViews(2, 1, &vertexAppendBufferUAV, &zero);

    // First execution of the compute shader, appends the indices of the nodes that should be checked next into the second buffer
    d3d11DevCon->Dispatch(1, 1, 1);

    d3d11DevCon->CopyStructureCount(structureCountBuffer, 0, secondBufferUAV);

    D3D11_MAPPED_SUBRESOURCE mappedSubresource;
    d3d11DevCon->Map(structureCountBuffer, 0, D3D11_MAP_READ, 0, &mappedSubresource);

    int structureCount = *(int*)mappedSubresource.pData;

    // Do as long as there is data in the output append buffer
    //while (structureCount > 0)
    //{
    //    //...
    //}

    // Create a vertex buffer description with dynamic write access
    D3D11_BUFFER_DESC vertexBufferDesc;
    ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));
    vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    vertexBufferDesc.ByteWidth = vertexBufferCount * sizeof(OctreeNodeVertex);
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexBufferDesc.CPUAccessFlags = 0;

    // Create the final vertex buffer
    hr = d3d11Device->CreateBuffer(&vertexBufferDesc, NULL, &vertexBuffer);
    ErrorMessage(L"CreateBuffer failed for the vertex buffer.", L"GetVertexBufferCompute", __FILEW__, __LINE__, hr);

    // Copy the vertices from the append buffer to the final vertex buffer on the GPU
    d3d11DevCon->CopyResource(vertexBuffer, vertexAppendBuffer);

    SAFE_RELEASE(firstBuffer);
    SAFE_RELEASE(secondBuffer);
    SAFE_RELEASE(vertexAppendBuffer);
    SAFE_RELEASE(structureCountBuffer);
    SAFE_RELEASE(firstBufferUAV);
    SAFE_RELEASE(secondBufferUAV);
    SAFE_RELEASE(vertexAppendBufferUAV);

    return vertexBuffer;
}
