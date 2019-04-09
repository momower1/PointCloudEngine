#include "Scene.h"

void Scene::Initialize()
{
    // Create and load the point cloud
    pointCloud = Hierarchy::Create(L"PointCloud");

    // Create loading text and hide it
    TextRenderer *loadingTextRenderer = new TextRenderer(TextRenderer::GetSpriteFont(L"Consolas"), false);
    loadingTextRenderer->text = L"Loading...";
    loadingText = Hierarchy::Create(L"Loading Text");
    loadingText->AddComponent(loadingTextRenderer);
    loadingText->transform->scale = Vector3::Zero;
    loadingText->transform->position = Vector3(-0.5f, 0.25f, 0.5f);

    // Create text renderer to display properties
    textRenderer = new TextRenderer(TextRenderer::GetSpriteFont(L"Consolas"), false);
    text = Hierarchy::Create(L"Text");
    text->AddComponent(textRenderer);

    // Transforms
    text->transform->position = Vector3(-1, 1, 0.5f);
    text->transform->scale = 0.35f * Vector3::One;
    camera->SetPosition(Vector3(0.0f, 2.5f, -3.0f));

    // Try to load the last plyfile
    DelayedLoadFile(settings->plyfile);
}

void Scene::Update(Timer &timer)
{
    // Toggle help
    if (Input::GetKeyDown(Keyboard::H))
    {
        help = !help;
    }

    // Interactively set the splat size in screen size
    if (Input::GetKey(Keyboard::Up))
    {
        splatSize += dt * 0.01f;
    }
    else if (Input::GetKey(Keyboard::Down))
    {
        splatSize -= dt * 0.01f;
    }

    splatSize = min(1.0f, max(1.0f / settings->resolutionY, splatSize));

    // Pass the splat size to the renderer
    if (pointCloudRenderer != NULL)
    {
        pointCloudRenderer->SetSplatSize(splatSize);
    }

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
    settings->scale = max(0.1f, settings->scale + Input::mouseScrollDelta);
    pointCloud->transform->scale = settings->scale * Vector3::One;

    // Rotate camera with mouse
    cameraYaw += dt * Input::mouseDelta.x;
    cameraPitch += dt * Input::mouseDelta.y;
    cameraPitch = cameraPitch > XM_PI / 2.1f ? XM_PI / 2.1f : (cameraPitch < -XM_PI / 2.1f ? -XM_PI / 2.1f : cameraPitch);
    camera->SetRotationMatrix(Matrix::CreateFromYawPitchRoll(cameraYaw, cameraPitch, 0));

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
    camera->TranslateRUF(cameraSpeed * dt * (Input::GetKey(Keyboard::D) - Input::GetKey(Keyboard::A)), 0, cameraSpeed * dt * (Input::GetKey(Keyboard::W) - Input::GetKey(Keyboard::S)));

    // FPS counter
    textRenderer->text = std::to_wstring(timer.GetFramesPerSecond()) + L" fps\n";

    // Show help / controls
    if (help)
    {
        textRenderer->text.append(L"[H] Toggle help\n");
        textRenderer->text.append(L"[O] Open .ply file with (x,y,z,nx,ny,nz,red,green,blue) format\n");
        textRenderer->text.append(L"[WASD] Move Camera\n");
        textRenderer->text.append(L"[MOUSE] Rotate Camera\n");
        textRenderer->text.append(L"[MOUSE WHEEL] Scale\n");
        textRenderer->text.append(L"[SPACE] Toggle rotation\n");
        textRenderer->text.append(L"[ENTER] Switch between octree splat/cube view\n");
        textRenderer->text.append(L"[UP/DOWN] Increase/decrease splat size\n");
        textRenderer->text.append(L"[RIGHT/LEFT] Increase/decrease octree level\n");
        textRenderer->text.append(L"[ESC] Quit application\n");
    }
    else
    {
        textRenderer->text.append(L"Press [H] to show help");
    }

    // Check if there is a file that should be loaded delayed
    if (timeUntilLoadFile > 0)
    {
        timeUntilLoadFile -= dt;

        if (timeUntilLoadFile < 0)
        {
            LoadFile();
        }
    }
    else if (Input::GetKeyDown(Keyboard::O))
    {
        // Open file dialog to load another file
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
            DelayedLoadFile(filename);
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
    Hierarchy::ReleaseAllSceneObjects();
}

void PointCloudEngine::Scene::DelayedLoadFile(std::wstring filepath)
{
    std::wifstream file(filepath);

    // Check if the file exists
    if (file.is_open())
    {
        // Load after some delay
        timeUntilLoadFile = 0.1f;
        settings->plyfile = filepath;

        // Show huge loading text
        loadingText->transform->scale = 1.5f * Vector3::One;
    }
}

void PointCloudEngine::Scene::LoadFile()
{
    // Release resources before loading
    if (pointCloudRenderer != NULL)
    {
        pointCloud->RemoveComponent(pointCloudRenderer);
    }

    std::vector<Vertex> vertices;

    // Try to load the file
    if (LoadPlyFile(vertices, settings->plyfile))
    {
        SetWindowTextW(hwnd, (settings->plyfile + L" - PointCloudEngine ").c_str());

        // Load the file (takes a long time)
        pointCloudRenderer = new RENDERER(vertices);
        pointCloud->AddComponent(pointCloudRenderer);
    }
    else
    {
        ErrorMessage(L"Could not open " + settings->plyfile + L"\nOnly .ply files with x,y,z,nx,ny,nz,red,green,blue vertex format are supported!", L"File loading error", __FILEW__, __LINE__);
    }

    // Hide loading text
    loadingText->transform->scale = Vector3::Zero;
}
