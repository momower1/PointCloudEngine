#include "Scene.h"

void Scene::Initialize()
{
    configFile = new ConfigFile();

    // Create text renderer to display properties
    textRenderer = new TextRenderer(TextRenderer::GetSpriteFont(L"Consolas"), false);
    text = Hierarchy::Create(L"Text");
    text->AddComponent(textRenderer);

    // Transforms
    text->transform->position = Vector3(-1, 1, 0.5f);
    text->transform->scale = 0.3f * Vector3::One;
    camera.position = Vector3(0.0f, 2.5f, -3.0f);

    Input::SetSensitivity(0.5f, 0.5f);

    // Create and load the point cloud
    pointCloud = Hierarchy::Create(L"PointCloud");

    std::wifstream plyfile (configFile->plyfile);

    if (plyfile.is_open())
    {
        SetWindowTextW(hwnd, (configFile->plyfile + L" - PointCloudEngine ").c_str());
        pointCloudRenderer = new RENDERER(configFile->plyfile);
        pointCloud->AddComponent(pointCloudRenderer);
    }
}

void Scene::Update(Timer &timer)
{
    if (Input::GetKeyDown(Keyboard::Space))
    {
        rotate = !rotate;
    }

    // Rotate the point cloud
    if (rotate)
    {
        pointCloud->transform->rotation *= Quaternion::CreateFromYawPitchRoll(dt / 2, 0, 0);
    }

    // Scale the point cloud by the value saved in the config file
    configFile->scale += Input::mouseScrollDelta;
    pointCloud->transform->scale = configFile->scale * Vector3::One;

    // Rotate camera with mouse
    cameraYaw += dt * Input::mouseDelta.x;
    cameraPitch += dt * Input::mouseDelta.y;
    cameraPitch = cameraPitch > XM_PI / 2.1f ? XM_PI / 2.1f : (cameraPitch < -XM_PI / 2.1f ? -XM_PI / 2.1f : cameraPitch);
    camera.CalculateRightUpForward(cameraPitch, cameraYaw);

    // Speed up camera when pressing shift
    if (Input::GetKey(Keyboard::LeftShift))
    {
        cameraSpeed += 20 * dt;
    }
    else
    {
        cameraSpeed = 3;
    }

    // Move camera with WASD keys
    camera.position += Input::GetKey(Keyboard::W) * cameraSpeed * dt * camera.forward;
    camera.position -= Input::GetKey(Keyboard::S) * cameraSpeed * dt * camera.forward;
    camera.position -= Input::GetKey(Keyboard::A) * cameraSpeed * dt * camera.right;
    camera.position += Input::GetKey(Keyboard::D) * cameraSpeed * dt * camera.right;

    // FPS counter
    textRenderer->text = std::to_wstring(timer.GetFramesPerSecond()) + L" fps\n";

    // Open file dialog to load another file
    if (Input::GetKeyDown(Keyboard::O))
    {
        wchar_t filename[MAX_PATH];
        OPENFILENAMEW openFileName;
        ZeroMemory(&openFileName, sizeof(OPENFILENAMEW));
        openFileName.lStructSize = sizeof(OPENFILENAMEW);
        openFileName.hwndOwner = hwnd;
        openFileName.lpstrFilter = L"Ply Files\0*.ply\0\0";
        openFileName.lpstrFile = filename;
        openFileName.lpstrFile[0] = L'\0';
        openFileName.nMaxFile = MAX_PATH;
        openFileName.lpstrTitle = L"Select a file to open!";
        openFileName.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
        openFileName.nFilterIndex = 1;

        Input::SetMode(Mouse::MODE_ABSOLUTE);

        if (GetOpenFileNameW(&openFileName))
        {
            configFile->plyfile = std::wstring(filename);
            SetWindowTextW(hwnd, (configFile->plyfile + L" - PointCloudEngine ").c_str());

            if (pointCloudRenderer != NULL)
            {
                pointCloud->RemoveComponent(pointCloudRenderer);
            }

            pointCloudRenderer = new RENDERER(configFile->plyfile);
            pointCloud->AddComponent(pointCloudRenderer);
        }

        Input::SetMode(Mouse::MODE_RELATIVE);
    }

    // Save config file and exit on ESC
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
    SafeDelete(configFile);

    Hierarchy::ReleaseAllSceneObjects();
}
