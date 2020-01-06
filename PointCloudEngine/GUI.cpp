#include "PrecompiledHeader.h"
#include "GUI.h"

UINT GUI::fps = 0;
UINT GUI::vertexCount = 0;
UINT GUI::cameraRecording = 0;

std::vector<Vector3> GUI::cameraRecordingPositions =
{
	Vector3(0, 0, -10),
	Vector3(0, 0, 10),
	Vector3(-10, 0, 0),
	Vector3(10, 0, 0),
	Vector3(0, 10, 0),
	Vector3(0, -10, 0)
};

std::vector<Matrix> GUI::cameraRecordingRotations =
{
	Matrix::CreateFromYawPitchRoll(0, 0, 0),
	Matrix::CreateFromYawPitchRoll(XM_PI, 0, 0),
	Matrix::CreateFromYawPitchRoll(XM_PI / 2, 0, 0),
	Matrix::CreateFromYawPitchRoll(-XM_PI / 2, 0, 0),
	Matrix::CreateFromYawPitchRoll(0, XM_PI / 2, 0),
	Matrix::CreateFromYawPitchRoll(0, -XM_PI / 2, 0)
};

bool GUI::waypointPreview = false;
float GUI::waypointPreviewLocation = 0;
WaypointRenderer* GUI::waypointRenderer = NULL;
GroundTruthRenderer* GUI::groundTruthRenderer = NULL;

bool GUI::initialized = false;
Vector3 GUI::waypointStartPosition;
Matrix GUI::waypointStartRotation;
Vector2 GUI::guiSize = Vector2(360, 440);

HWND GUI::hwndGUI = NULL;
GUITab* GUI::tabGroundTruth = NULL;
GUITab* GUI::tabOctree = NULL;

// General
std::vector<IGUIElement*> GUI::generalElements;
std::vector<IGUIElement*> GUI::splatElements;
std::vector<IGUIElement*> GUI::octreeElements;
std::vector<IGUIElement*> GUI::sparseElements;
std::vector<IGUIElement*> GUI::neuralNetworkElements;

// Advanced
std::vector<IGUIElement*> GUI::advancedElements;

// HDF5
std::vector<IGUIElement*> GUI::hdf5Elements;

void PointCloudEngine::GUI::Initialize()
{
	if (!initialized)
	{
		// Place the gui window for changing settings next to the main window
		RECT rect;
		GetWindowRect(hwnd, &rect);

		// Initialize common controls library to use buttons and so on
		InitCommonControls();

		// This is the gui window
		hwndGUI = CreateWindowEx(NULL, L"PointCloudEngine", L"Settings", WS_SYSMENU | WS_CAPTION | WS_VISIBLE, rect.right, rect.top, guiSize.x, guiSize.y, hwnd, NULL, NULL, NULL);

		// Tab inside the gui window for choosing different groups of settings
		tabGroundTruth = new GUITab(hwndGUI, XMUINT2(0, 0), XMUINT2(guiSize.x, guiSize.y), { L"General", L"Advanced", L"HDF5 Dataset" }, OnSelectTab);
		tabOctree = new GUITab(hwndGUI, XMUINT2(0, 0), XMUINT2(guiSize.x, guiSize.y), { L"General", L"Advanced" }, OnSelectTab);

		// Create and show content for each tab
		CreateContentGeneral();
		CreateContentAdvanced();
		CreateContentHDF5();

		initialized = true;
	}

	OnSelectTab(0);
	OnSelectViewMode();
}

void PointCloudEngine::GUI::Release()
{
	initialized = false;

	// SafeDelete all GUI elements, HWNDs are released automatically
	SafeDelete(tabGroundTruth);
	SafeDelete(tabOctree);
	DeleteElements(generalElements);
	DeleteElements(splatElements);
	DeleteElements(octreeElements);
	DeleteElements(sparseElements);
	DeleteElements(neuralNetworkElements);
	DeleteElements(advancedElements);
	DeleteElements(hdf5Elements);
}

void PointCloudEngine::GUI::Update()
{
	if (!initialized)
		return;

	tabGroundTruth->Update();
	tabOctree->Update();
	UpdateElements(generalElements);
	UpdateElements(splatElements);
	UpdateElements(octreeElements);
	UpdateElements(sparseElements);
	UpdateElements(neuralNetworkElements);
	UpdateElements(advancedElements);
	UpdateElements(hdf5Elements);
}

void PointCloudEngine::GUI::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (!initialized)
		return;

	// WindowsProcess message will be forwarded to this function
	// The GUI is only interested in these kinds of messages for e.g. button notifications
	if (msg == WM_COMMAND || msg == WM_NOTIFY || msg == WM_HSCROLL)
	{
		tabGroundTruth->HandleMessage(msg, wParam, lParam);
		tabOctree->HandleMessage(msg, wParam, lParam);
		HandleMessageElements(generalElements, msg, wParam, lParam);
		HandleMessageElements(splatElements, msg, wParam, lParam);
		HandleMessageElements(octreeElements, msg, wParam, lParam);
		HandleMessageElements(sparseElements, msg, wParam, lParam);
		HandleMessageElements(neuralNetworkElements, msg, wParam, lParam);
		HandleMessageElements(advancedElements, msg, wParam, lParam);
		HandleMessageElements(hdf5Elements, msg, wParam, lParam);
	}
}

void PointCloudEngine::GUI::SetVisible(bool visible)
{
	ShowWindow(hwndGUI, visible ? SW_SHOW : SW_HIDE);
	
	if (settings->useOctree)
	{
		tabOctree->Show(SW_SHOW);
		tabGroundTruth->Show(SW_HIDE);
	}
	else
	{
		tabGroundTruth->Show(SW_SHOW);
		tabOctree->Show(SW_HIDE);
	}
}

void PointCloudEngine::GUI::ShowElements(std::vector<IGUIElement*> elements, int SW_COMMAND)
{
	for (auto it = elements.begin(); it != elements.end(); it++)
	{
		(*it)->Show(SW_COMMAND);
	}
}

void PointCloudEngine::GUI::DeleteElements(std::vector<IGUIElement*> elements)
{
	for (auto it = elements.begin(); it != elements.end(); it++)
	{
		SafeDelete(*it);
	}

	elements.clear();
}

void PointCloudEngine::GUI::UpdateElements(std::vector<IGUIElement*> elements)
{
	for (auto it = elements.begin(); it != elements.end(); it++)
	{
		(*it)->Update();
	}
}

void PointCloudEngine::GUI::HandleMessageElements(std::vector<IGUIElement*> elements, UINT msg, WPARAM wParam, LPARAM lParam)
{
	for (auto it = elements.begin(); it != elements.end(); it++)
	{
		(*it)->HandleMessage(msg, wParam, lParam);
	}
}

void PointCloudEngine::GUI::CreateContentGeneral()
{
	generalElements.push_back(new GUIText(hwndGUI, { 10, 40 }, { 100, 20 }, L"View Mode "));
	generalElements.push_back(new GUIDropdown(hwndGUI, { 160, 35 }, { 130, 200 }, { L"Splats", L"Sparse Splats", L"Points", L"Sparse Points", L"Neural Network" }, OnSelectViewMode, &settings->viewMode));
	generalElements.push_back(new GUIDropdown(hwndGUI, { 160, 35 }, { 130, 200 }, { L"Splats", L"Octree Nodes", L"Normal Clusters" }, OnSelectViewMode, &settings->viewMode));
	generalElements.push_back(new GUIText(hwndGUI, { 10, 70 }, { 150, 20 }, L"Vertex Count "));
	generalElements.push_back(new GUIValue<UINT>(hwndGUI, { 160, 70 }, { 200, 20 }, &GUI::vertexCount));
	generalElements.push_back(new GUIText(hwndGUI, { 10, 100 }, { 150, 20 }, L"Frames per second "));
	generalElements.push_back(new GUIValue<UINT>(hwndGUI, { 160, 100 }, { 50, 20 }, &GUI::fps));
	generalElements.push_back(new GUIText(hwndGUI, { 10, 130 }, { 150, 20 }, L"Lighting "));
	generalElements.push_back(new GUICheckbox(hwndGUI, { 160, 130 }, { 20, 20 }, L"", NULL, &settings->useLighting));

	splatElements.push_back(new GUIText(hwndGUI, { 10, 160 }, { 150, 20 }, L"Blending "));
	splatElements.push_back(new GUICheckbox(hwndGUI, { 160, 160 }, { 20, 20 }, L"", NULL, &settings->useBlending));
	splatElements.push_back(new GUISlider<float>(hwndGUI, { 160, 190 }, { 130, 20 }, { 0, 100 }, 10, 0, L"Blend Factor", &settings->blendFactor));
	splatElements.push_back(new GUISlider<float>(hwndGUI, { 160, 220 }, { 130, 20 }, { 0, 1000 }, 1000, 0, L"Sampling Rate", &settings->samplingRate));

	octreeElements.push_back(new GUISlider<float>(hwndGUI, { 160, 250 }, { 130, 20 }, { 4, 100 }, settings->resolutionY * 4, 0, L"Splat Resolution", &settings->splatResolution));
	octreeElements.push_back(new GUISlider<float>(hwndGUI, { 160, 280 }, { 130, 20 }, { 0, 500 }, 100, -100, L"Overlap Factor", &settings->overlapFactor));
	octreeElements.push_back(new GUISlider<int>(hwndGUI, { 160, 310 }, { 130, 20 }, { 0, 1 + (UINT)settings->maxOctreeDepth }, 1, 1, L"Octree Level", &settings->octreeLevel));
	octreeElements.push_back(new GUIText(hwndGUI, { 10, 340 }, { 150, 20 }, L"Culling "));
	octreeElements.push_back(new GUICheckbox(hwndGUI, { 160, 340 }, { 20, 20 }, L"", NULL, &settings->useCulling));
	octreeElements.push_back(new GUIText(hwndGUI, { 10, 370 }, { 150, 20 }, L"GPU Traversal "));
	octreeElements.push_back(new GUICheckbox(hwndGUI, { 160, 370 }, { 20, 20 }, L"", NULL, &settings->useGPUTraversal));

	sparseElements.push_back(new GUISlider<float>(hwndGUI, { 160, 220 }, { 130, 20 }, { 0, 1000 }, 100, 0, L"Sparse Sampling Rate", &settings->sparseSamplingRate));
	sparseElements.push_back(new GUISlider<float>(hwndGUI, { 160, 250 }, { 130, 20 }, { 0, 1000 }, 1000, 0, L"Density", &settings->density));
}

void PointCloudEngine::GUI::CreateContentAdvanced()
{
	advancedElements.push_back(new GUISlider<float>(hwndGUI, { 160, 40 }, { 130, 20 }, { 1, 1000 }, 100, 0, L"Scale", &settings->scale));
	advancedElements.push_back(new GUIText(hwndGUI, { 10, 70 }, { 150, 20 }, L"Background RGB"));
	advancedElements.push_back(new GUISlider<float>(hwndGUI, { 160, 70 }, { 45, 20 }, { 0, 100 }, 100, 0, L"R", &settings->backgroundColor.x, 0, 0));
	advancedElements.push_back(new GUISlider<float>(hwndGUI, { 203, 70 }, { 45, 20 }, { 0, 100 }, 100, 0, L"G", &settings->backgroundColor.y, 0, 0));
	advancedElements.push_back(new GUISlider<float>(hwndGUI, { 245, 70 }, { 45, 20 }, { 0, 100 }, 100, 0, L"B", &settings->backgroundColor.z, 0, 0));

	advancedElements.push_back(new GUIText(hwndGUI, { 10, 130 }, { 150, 20 }, L"Headlight "));
	advancedElements.push_back(new GUICheckbox(hwndGUI, { 160, 130 }, { 20, 20 }, L"", NULL, &settings->useHeadlight));
	advancedElements.push_back(new GUIText(hwndGUI, { 10, 160 }, { 150, 20 }, L"Light Direction XYZ"));
	advancedElements.push_back(new GUISlider<float>(hwndGUI, { 160, 160 }, { 45, 20 }, { 0, 200 }, 100, 100, L"X", &settings->lightDirection.x, 0, 0));
	advancedElements.push_back(new GUISlider<float>(hwndGUI, { 203, 160 }, { 45, 20 }, { 0, 200 }, 100, 100, L"Y", &settings->lightDirection.y, 0, 0));
	advancedElements.push_back(new GUISlider<float>(hwndGUI, { 245, 160 }, { 45, 20 }, { 0, 200 }, 100, 100, L"Z", &settings->lightDirection.z, 0, 0));
	advancedElements.push_back(new GUISlider<float>(hwndGUI, { 160, 190 }, { 130, 20 }, { 0, 200 }, 100, 0, L"Lighting Ambient", &settings->ambient));
	advancedElements.push_back(new GUISlider<float>(hwndGUI, { 160, 220 }, { 130, 20 }, { 0, 200 }, 100, 0, L"Lighting Diffuse", &settings->diffuse));
	advancedElements.push_back(new GUISlider<float>(hwndGUI, { 160, 250 }, { 130, 20 }, { 0, 200 }, 100, 0, L"Lighting Specular", &settings->specular));
	advancedElements.push_back(new GUISlider<float>(hwndGUI, { 160, 280 }, { 130, 20 }, { 0, 2000 }, 100, 0, L"Lighting Specular Exp.", &settings->specularExponent));

	advancedElements.push_back(new GUIButton(hwndGUI, { 10, 330 }, { 150, 25 }, L"Add Waypoint", OnWaypointAdd));
	advancedElements.push_back(new GUIButton(hwndGUI, { 180, 330 }, { 150, 25 }, L"Remove Waypoint", OnWaypointRemove));
	advancedElements.push_back(new GUIButton(hwndGUI, { 10, 365 }, { 150, 25 }, L"Toggle Waypoints", OnWaypointToggle));
	advancedElements.push_back(new GUIButton(hwndGUI, { 180, 365 }, { 150, 25 }, L"Preview Waypoints", OnWaypointPreview));
}

void PointCloudEngine::GUI::CreateContentHDF5()
{
	// TODO: Use this to select a camera position from the HDF5 file generation
	hdf5Elements.push_back(new GUISlider<UINT>(hwndGUI, { 160, 40 }, { 130, 20 }, { 1, (UINT)cameraRecordingPositions.size() }, 1, 0, L"Camera Recording", &cameraRecording, 148, 43, OnChangeCameraRecording));

	hdf5Elements.push_back(new GUIText(hwndGUI, { 10, 80 }, { 300, 20 }, L"Waypoint Dataset Generation"));
	hdf5Elements.push_back(new GUISlider<float>(hwndGUI, { 160, 110 }, { 130, 20 }, { 1, 1000 }, 1000, 0, L"Step Size", &settings->waypointStepSize));
	hdf5Elements.push_back(new GUISlider<float>(hwndGUI, { 160, 140 }, { 130, 20 }, { 1, 1000 }, 1000, 0, L"Preview Step Size", &settings->waypointPreviewStepSize));
	hdf5Elements.push_back(new GUIText(hwndGUI, { 10, 170 }, { 150, 20 }, L"Range Min/Max"));
	hdf5Elements.push_back(new GUISlider<float>(hwndGUI, { 160, 170 }, { 50, 20 }, { 0, 100 }, 100, 0, L"Min", &settings->waypointMin, 0, 28));
	hdf5Elements.push_back(new GUISlider<float>(hwndGUI, { 240, 170 }, { 50, 20 }, { 0, 100 }, 100, 0, L"Max", &settings->waypointMax, 0, 28));
	hdf5Elements.push_back(new GUIButton(hwndGUI, { 10, 200 }, { 325, 25 }, L"Generate Waypoint HDF5 Dataset", OnGenerateWaypointDataset));

	hdf5Elements.push_back(new GUIText(hwndGUI, { 10, 245 }, { 300, 20 }, L"Sphere Dataset Generation"));
	hdf5Elements.push_back(new GUISlider<float>(hwndGUI, { 160, 275 }, { 130, 20 }, { 1, 6283 }, 1000, 0, L"Sphere Step Size", &settings->sphereStepSize));
	hdf5Elements.push_back(new GUIText(hwndGUI, { 10, 305 }, { 150, 20 }, L"Theta Min/Max"));
	hdf5Elements.push_back(new GUISlider<float>(hwndGUI, { 160, 305 }, { 50, 20 }, { 1, 628 }, 100, 0, L"Min", &settings->sphereMinTheta, 0, 28));
	hdf5Elements.push_back(new GUISlider<float>(hwndGUI, { 240, 305 }, { 50, 20 }, { 1, 628 }, 100, 0, L"Max", &settings->sphereMaxTheta, 0, 28));
	hdf5Elements.push_back(new GUIText(hwndGUI, { 10, 335 }, { 150, 20 }, L"Phi Min/Max"));
	hdf5Elements.push_back(new GUISlider<float>(hwndGUI, { 160, 335 }, { 50, 20 }, { 1, 628 }, 100, 0, L"Min", &settings->sphereMinPhi, 0, 28));
	hdf5Elements.push_back(new GUISlider<float>(hwndGUI, { 240, 335 }, { 50, 20 }, { 1, 628 }, 100, 0, L"Max", &settings->sphereMaxPhi, 0, 28));
	hdf5Elements.push_back(new GUIButton(hwndGUI, { 10, 365 }, { 325, 25 }, L"Generate Sphere HDF5 Dataset", OnGenerateSphereDataset));
}

void PointCloudEngine::GUI::OnSelectViewMode()
{
	ShowElements(octreeElements, SW_HIDE);
	ShowElements(splatElements, SW_HIDE);
	ShowElements(sparseElements, SW_HIDE);
	ShowElements(neuralNetworkElements, SW_HIDE);

	if (settings->useOctree)
	{
		ShowElements(splatElements);
		ShowElements(octreeElements);
	}
	else
	{
		switch (settings->viewMode)
		{
			case 0:
			{
				ShowElements(splatElements);
				break;
			}
			case 1:
			{
				ShowElements(splatElements);
				ShowElements(sparseElements);
				splatElements[3]->Show(SW_HIDE);
				sparseElements[1]->SetPosition({ 160, 250 });
				break;
			}
			case 3:
			{
				sparseElements[1]->Show(SW_SHOW);
				sparseElements[1]->SetPosition({ 160, 160 });
				break;
			}
			case 4:
			{
				ShowElements(neuralNetworkElements);
				break;
			}
		}
	}
}

void PointCloudEngine::GUI::OnSelectTab(int selection)
{
	ShowElements(generalElements, SW_HIDE);
	ShowElements(splatElements, SW_HIDE);
	ShowElements(octreeElements, SW_HIDE);
	ShowElements(sparseElements, SW_HIDE);
	ShowElements(neuralNetworkElements, SW_HIDE);
	ShowElements(advancedElements, SW_HIDE);
	ShowElements(hdf5Elements, SW_HIDE);

	switch (selection)
	{
		case 0:
		{
			ShowElements(generalElements);
			OnSelectViewMode();

			if (settings->useOctree)
			{
				generalElements[1]->Show(SW_HIDE);
			}
			else
			{
				generalElements[2]->Show(SW_HIDE);
			}
			break;
		}
		case 1:
		{
			ShowElements(advancedElements);
			break;
		}
		case 2:
		{
			ShowElements(hdf5Elements);
			break;
		}
	}
}

void PointCloudEngine::GUI::OnWaypointAdd()
{
	if (waypointRenderer != NULL)
	{
		waypointRenderer->AddWaypoint(camera->GetPosition(), camera->GetRotationMatrix(), camera->GetForward());
	}
}

void PointCloudEngine::GUI::OnWaypointRemove()
{
	if (waypointRenderer != NULL)
	{
		waypointRenderer->RemoveWaypoint();
	}
}

void PointCloudEngine::GUI::OnWaypointToggle()
{
	if (waypointRenderer != NULL)
	{
		waypointRenderer->enabled = !waypointRenderer->enabled;
	}
}

void PointCloudEngine::GUI::OnWaypointPreview()
{
	if (waypointRenderer != NULL)
	{
		waypointPreview = !waypointPreview;

		// Camera tracking shot using the waypoints
		if (waypointPreview)
		{
			// Start preview
			waypointPreviewLocation = 0;
			waypointStartPosition = camera->GetPosition();
			waypointStartRotation = camera->GetRotationMatrix();
		}
		else
		{
			// End of preview
			camera->SetPosition(waypointStartPosition);
			camera->SetRotationMatrix(waypointStartRotation);
		}
	}
}

void PointCloudEngine::GUI::OnGenerateWaypointDataset()
{
	if (groundTruthRenderer != NULL)
	{
		groundTruthRenderer->GenerateWaypointDataset();
	}
}

void PointCloudEngine::GUI::OnGenerateSphereDataset()
{
	if (groundTruthRenderer != NULL)
	{
		groundTruthRenderer->GenerateSphereDataset();
	}
}

void PointCloudEngine::GUI::OnChangeCameraRecording()
{
	camera->SetPosition(cameraRecordingPositions[cameraRecording - 1]);
	camera->SetRotationMatrix(cameraRecordingRotations[cameraRecording - 1]);
}
