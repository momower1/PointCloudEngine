#include "Scene.h"

void Scene::Initialize()
{
    pointCloud = Hierarchy::Create(L"PointCloud");
    pointCloud->AddComponent(PointCloudRenderer::CreateShared(L"Assets/stanford_dragon_xyz_rgba_normals.ply"));

    fpsText = Hierarchy::Create(L"Text");
    fpsText->AddComponent(new TextRenderer(TextRenderer::GetSpriteFont(L"Consolas"), false));

    debugText = Hierarchy::Create(L"Debug Text");
    TextRenderer *debugTextRenderer = debugText->AddComponent(new TextRenderer(TextRenderer::GetSpriteFont(L"Times New Roman"), true));
    debugTextRenderer->text = L"Welcome!\nThis is 3D text in PointCloudEngine!\nTODO!";

    // Transforms
    camera.position = Vector3(0.0f, 2.0f, -5.0f);

    // Text Transforms
    debugText->transform->position = Vector3(0, 3, 3);
    debugText->transform->scale = Vector3::One;
    fpsText->transform->position = Vector3(-1, 1, 0.5f);
    fpsText->transform->scale = 0.3f * Vector3::One;
}

void Scene::Update(Timer &timer)
{
    // Rotate camera with mouse
    cameraYaw += dt * Input::mouseDelta.x;
    cameraPitch += dt * Input::mouseDelta.y;
    cameraPitch = cameraPitch > XM_PI / 2.1f ? XM_PI / 2.1f : (cameraPitch < -XM_PI / 2.1f ? -XM_PI / 2.1f : cameraPitch);
    camera.CalculateRightUpForward(cameraPitch, cameraYaw);

    // Move camera with arrow keys
    float cameraSpeed = Input::GetKey(Keyboard::LeftShift) ? 30 : 10;
    camera.position += Input::GetKey(Keyboard::W) * cameraSpeed * dt * camera.forward;
    camera.position -= Input::GetKey(Keyboard::S) * cameraSpeed * dt * camera.forward;
    camera.position -= Input::GetKey(Keyboard::A) * cameraSpeed * dt * camera.right;
    camera.position += Input::GetKey(Keyboard::D) * cameraSpeed * dt * camera.right;

    // FPS counter
    fpsText->GetComponent<TextRenderer>()->text = std::to_wstring(timer.GetFramesPerSecond()) + L" fps";

    // Exit on ESC
    if (Input::GetKeyDown(Keyboard::Escape))
    {
        DestroyWindow(hwnd);
    }

    Hierarchy::UpdateAllSceneObjects();
}

void Scene::Draw()
{
    Hierarchy::DrawAllSceneObjects();
}

void Scene::Release()
{
    Hierarchy::ReleaseAllSceneObjects();
    PointCloudRenderer::ReleaseAllSharedPointCloudRenderers();
}
