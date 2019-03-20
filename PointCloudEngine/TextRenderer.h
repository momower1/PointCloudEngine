#ifndef TEXTRENDERER_H
#define TEXTRENDERER_H

#pragma once
#include "PointCloudEngine.h"

namespace PointCloudEngine
{
    class TextRenderer : public Component
    {
    public:
        // Static functions to create shared fonts
        static void CreateSpriteFont(std::wstring fontName, std::wstring filename);
        static SpriteFont* GetSpriteFont (std::wstring fontName);
        static void ReleaseAllSpriteFonts();

        TextRenderer (SpriteFont *spritefont, bool worldSpace);
        void Initialize(SceneObject *sceneObject);
        void Update(SceneObject *sceneObject);
        void Draw (SceneObject *sceneObject);
        void Release();

        bool worldSpace = false;
        std::wstring text = L"Text";
        Color color = Color (1.0f, 1.0f, 1.0f, 1.0f);

    private:
        static std::map<std::wstring, SpriteFont*> fonts;

        // Check if the text changed and only create a new vertex buffer in that case
        std::wstring oldText = L"";

        SpriteFont *spriteFont;

        struct TextVertex
        {
            Vector2 position;
            Vector3 offset;     //z: xAdvance
            Vector4 rect;       //x:left, y:top, z:right, w:bottom
        };

        struct ConstantBufferText
        {
            int worldSpace;
            int padding[3];
            Color color;
            Matrix World;
            Matrix View;
            Matrix Projection;
        };

        // Sprite sheet texture
        ID3D11ShaderResourceView *shaderResourceView;

        ID3D11Buffer* vertexBuffer = NULL;
        std::vector<TextVertex> vertices;

        ID3D11Buffer* constantBufferText;
    };
}
#endif
