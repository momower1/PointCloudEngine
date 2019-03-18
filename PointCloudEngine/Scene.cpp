#include "Scene.h"

void Scene::Initialize()
{
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

    cameraDistance = 1;

    // Load ply file
    std::ifstream ss(L"Assets/stanford_dragon_xyz_rgba_normals.ply", std::ios::binary);

    PlyFile file;
    file.parse_header(ss);

    // Tinyply untyped byte buffers for properties
    std::shared_ptr<PlyData> rawPositions, rawNormals, rawColors;

    // Hardcoded properties and elements
    rawPositions = file.request_properties_from_element("vertex", { "x", "y", "z" });
    rawNormals = file.request_properties_from_element("vertex", { "nx", "ny", "nz" });
    rawColors = file.request_properties_from_element("vertex", { "red", "green", "blue", "alpha" });

    // Read the file
    file.read(ss);

    // Cast the buffers
    const size_t sizePositions = rawPositions->buffer.size_bytes();
    std::vector<Vector3> positions (rawPositions->count);
    std::memcpy(positions.data(), rawPositions->buffer.get(), sizePositions);

    const size_t sizeNormals = rawNormals->buffer.size_bytes();
    std::vector<Vector3> normals(rawNormals->count);
    std::memcpy(normals.data(), rawNormals->buffer.get(), sizeNormals);

    // TODO: change representation into maybe an int32 that stores rgba
    const size_t sizeColors = rawColors->buffer.size_bytes();
    std::vector<BYTE> colors(rawColors->count * 4);
    std::memcpy(colors.data(), rawColors->buffer.get(), sizeColors);

    // ...
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
}
