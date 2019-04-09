#include "TextRenderer.h"

std::map<std::wstring, SpriteFont*> TextRenderer::fonts;

void TextRenderer::CreateSpriteFont(std::wstring fontName, std::wstring filename)
{
    fonts[fontName] = new SpriteFont(d3d11Device, filename.c_str());
}

SpriteFont* TextRenderer::GetSpriteFont(std::wstring fontName)
{
    return fonts[fontName];
}

void TextRenderer::ReleaseAllSpriteFonts()
{
    for (auto it = fonts.begin(); it != fonts.end(); it++)
    {
        delete (*it).second;
    }

    fonts.clear();
}

TextRenderer::TextRenderer(SpriteFont *spriteFont, bool worldSpace)
{
    this->worldSpace = worldSpace;
    this->spriteFont = spriteFont;

    // Get texture
    spriteFont->GetSpriteSheet(&shaderResourceView);
    spriteFont->SetDefaultCharacter('?');
}

void TextRenderer::Initialize(SceneObject *sceneObject)
{
    // Create the constant buffer for WVP
    D3D11_BUFFER_DESC cbDescWVP;
    ZeroMemory(&cbDescWVP, sizeof(cbDescWVP));
    cbDescWVP.Usage = D3D11_USAGE_DEFAULT;
    cbDescWVP.ByteWidth = sizeof(ConstantBufferText);
    cbDescWVP.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDescWVP.CPUAccessFlags = 0;
    cbDescWVP.MiscFlags = 0;

    hr = d3d11Device->CreateBuffer(&cbDescWVP, NULL, &constantBufferText);
    ErrorMessage(L"CreateBuffer failed for the constant buffer matrices.", L"Initialize", __FILEW__, __LINE__, hr);
}

void TextRenderer::Update(SceneObject *sceneObject)
{
    // Create vertex buffer if the text changed
    if (text != oldText)
    {
        SafeRelease(vertexBuffer);
        vertices.clear();

        Vector2 glyphPosition = Vector2::Zero;

        // Each vertex stores its text position and offset in the sprite font texture
        // Then the geometry shader creates a rect at that position and samples the character from the glyph offset and rect
        for (int i = 0; i < text.length(); i++)
        {
            if (text[i] == '\n')
            {
                glyphPosition = Vector2(0, glyphPosition.y - spriteFont->GetLineSpacing());
                continue;
            }

            const SpriteFont::Glyph *glyph = spriteFont->FindGlyph(text[i]);

            TextVertex textVertex;
            textVertex.position = glyphPosition;
            textVertex.offset = Vector3(glyph->XOffset, glyph->YOffset, glyph->XAdvance);
            textVertex.rect = Vector4(glyph->Subrect.left, glyph->Subrect.top, glyph->Subrect.right, glyph->Subrect.bottom);

            float width = glyph->Subrect.right - glyph->Subrect.left;
            glyphPosition.x += width + textVertex.offset.x;
            glyphPosition.x += glyph->XAdvance;

            vertices.push_back(textVertex);
        }

        // Create a vertex buffer description
        D3D11_BUFFER_DESC vertexBufferDesc;
        ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));
        vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
        vertexBufferDesc.ByteWidth = sizeof(TextVertex) * vertices.size();
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

        oldText = text;
    }
}

void TextRenderer::Draw(SceneObject *sceneObject)
{
    // Set the shaders
    d3d11DevCon->VSSetShader(textShader->vertexShader, 0, 0);
    d3d11DevCon->GSSetShader(textShader->geometryShader, 0, 0);
    d3d11DevCon->PSSetShader(textShader->pixelShader, 0, 0);

    // Set the Input (Vertex) Layout
    d3d11DevCon->IASetInputLayout(textShader->inputLayout);

    // Bind the vertex buffer and index buffer to the input assembler (IA)
    UINT stride = sizeof(TextVertex);
    UINT offset = 0;
    d3d11DevCon->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);

    // Set primitive topology
    d3d11DevCon->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

    // Sprite sheet texture
    d3d11DevCon->PSSetShaderResources(0, 1, &shaderResourceView);

    // Set shader constant buffer variables
    ConstantBufferText tmp;
    tmp.worldSpace = worldSpace;
    tmp.color = color;
    tmp.World = sceneObject->transform->worldMatrix.Transpose();
    tmp.View = camera->GetViewMatrix().Transpose();
    tmp.Projection = camera->GetProjectionMatrix().Transpose();

    d3d11DevCon->UpdateSubresource(constantBufferText, 0, NULL, &tmp, 0, 0);		// Update effect file buffer

    // Set shader buffer to our created buffer
    d3d11DevCon->VSSetConstantBuffers(0, 1, &constantBufferText);
    d3d11DevCon->GSSetConstantBuffers(0, 1, &constantBufferText);
    d3d11DevCon->PSSetConstantBuffers(0, 1, &constantBufferText);

    d3d11DevCon->Draw(vertices.size(), 0);

    // Unset the geometry shader
    d3d11DevCon->GSSetShader(NULL, 0, 0);
}

void TextRenderer::Release()
{
    shaderResourceView->Release();
    SafeRelease(vertexBuffer);
}
