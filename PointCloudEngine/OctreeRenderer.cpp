#include "OctreeRenderer.h"

OctreeRenderer::OctreeRenderer(std::wstring plyfile)
{
    std::vector<Vertex> vertices = LoadPlyFile(plyfile);

    // Create the octree
    octree = new Octree(vertices, 12);

    // Text for showing properties
    text = Hierarchy::Create(L"OctreeRendererText");
    textRenderer = text->AddComponent(new TextRenderer(TextRenderer::GetSpriteFont(L"Consolas"), false));

    text->transform->position = Vector3(-1, -0.95, 0);
    text->transform->scale = 0.3f * Vector3::One;

    // Don't use specific view direction, use camera view direction
    constantBufferData.viewDirectionIndex = -1;
    constantBufferData.fovAngleY = fovAngleY;
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

    // Select view direction with F1 to F6 keys
    for (int i = 0; i < 6; i++)
    {
        if (Input::GetKeyDown((Keyboard::Keys)(Keyboard::F1 + i)))
        {
            // Reset to camera view direction if the same key is pressed twice
            if (constantBufferData.viewDirectionIndex == i)
            {
                constantBufferData.viewDirectionIndex = -1;
            }
            else
            {
                constantBufferData.viewDirectionIndex = i;
            }
        }
    }

    // Toggle draw splats
    if (Input::GetKeyDown(Keyboard::Enter))
    {
        drawSplats = !drawSplats;
    }

    // Create new buffer from the current octree traversal
    if (level < 0)
    {
        Matrix worldInverse = sceneObject->transform->worldMatrix.Invert();
        Vector3 localCameraPosition = Vector4::Transform(Vector4(camera.position.x, camera.position.y, camera.position.z, 1), worldInverse);

        octreeVertices = octree->GetVertices(localCameraPosition, constantBufferData.splatSize);
    }
    else
    {
        octreeVertices = octree->GetVerticesAtLevel(level);
    }

    // Set the text
    textRenderer->text = L"Octree Level: " + std::to_wstring(level) + L", VertexBuffer: " + std::to_wstring(octreeVertices.size()) + L"/" + std::to_wstring(vertexBufferSize);
}

void OctreeRenderer::Draw(SceneObject *sceneObject)
{
    int octreeVerticesSize = octreeVertices.size();

    if (octreeVerticesSize > 0)
    {
        // The vertex buffer should be recreated once the octree vertex count is larger than the current buffer
        // It might be good to recreate it as well when the octree vertex count is much smaller to save gpu memory
        if (octreeVerticesSize > vertexBufferSize)
        {
            // Release vertex buffer
            SafeRelease(vertexBuffer);

            // Create a vertex buffer description with dynamic write access
            D3D11_BUFFER_DESC vertexBufferDesc;
            ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));
            vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
            vertexBufferDesc.ByteWidth = sizeof(OctreeNodeVertex) * octreeVerticesSize;
            vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

            // Fill a D3D11_SUBRESOURCE_DATA struct with the data we want in the buffer
            D3D11_SUBRESOURCE_DATA vertexBufferData;
            ZeroMemory(&vertexBufferData, sizeof(vertexBufferData));
            vertexBufferData.pSysMem = &octreeVertices[0];

            // Create the buffer
            hr = d3d11Device->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &vertexBuffer);
            ErrorMessage(L"CreateBuffer failed for the vertex buffer.", L"Initialize", __FILEW__, __LINE__, hr);

            vertexBufferSize = octreeVerticesSize;
        }
        else
        {
            // Just update the dynamic buffer
            D3D11_MAPPED_SUBRESOURCE mappedVertexBuffer;
            ZeroMemory(&mappedVertexBuffer, sizeof(D3D11_MAPPED_SUBRESOURCE));

            // Disable GPU access to the vertex buffer data
            d3d11DevCon->Map(vertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedVertexBuffer);

            // Update vertex buffer data
            memcpy(mappedVertexBuffer.pData, &octreeVertices[0], octreeVerticesSize * sizeof(OctreeNodeVertex));

            // Reenable GPU access
            d3d11DevCon->Unmap(vertexBuffer, 0);
        }

        // Set the shaders
        if (drawSplats)
        {
            d3d11DevCon->VSSetShader(octreeSplatShader->vertexShader, 0, 0);
            d3d11DevCon->GSSetShader(octreeSplatShader->geometryShader, 0, 0);
            d3d11DevCon->PSSetShader(octreeSplatShader->pixelShader, 0, 0);
        }
        else
        {
            d3d11DevCon->VSSetShader(octreeCubeShader->vertexShader, 0, 0);
            d3d11DevCon->GSSetShader(octreeCubeShader->geometryShader, 0, 0);
            d3d11DevCon->PSSetShader(octreeCubeShader->pixelShader, 0, 0);
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
        constantBufferData.View = camera.view.Transpose();
        constantBufferData.Projection = camera.projection.Transpose();
        constantBufferData.cameraPosition = camera.position;

        // Update effect file buffer, set shader buffer to our created buffer
        d3d11DevCon->UpdateSubresource(constantBuffer, 0, NULL, &constantBufferData, 0, 0);
        d3d11DevCon->VSSetConstantBuffers(0, 1, &constantBuffer);
        d3d11DevCon->GSSetConstantBuffers(0, 1, &constantBuffer);

        d3d11DevCon->Draw(octreeVerticesSize, 0);
    }
}

void OctreeRenderer::Release()
{
    SafeDelete(octree);

    Hierarchy::ReleaseSceneObject(text);

    SafeRelease(vertexBuffer);
    SafeRelease(constantBuffer);
}

void PointCloudEngine::OctreeRenderer::SetSplatSize(const float &splatSize)
{
    constantBufferData.splatSize = splatSize;
}
