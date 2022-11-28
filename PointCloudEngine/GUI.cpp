#include "PrecompiledHeader.h"
#include "GUI.h"

#define CAMERARECORDINGS_FILENAME L"/CameraRecordings.vector"

UINT GUI::fps = 0;
UINT GUI::vertexCount = 0;
UINT GUI::cameraRecording = 0;
int GUI::lossFunctionSelection = 0;
float GUI::l1Loss = 0;
float GUI::mseLoss = 0;
float GUI::smoothL1Loss = 0;

HFONT IGUIElement::hFont = NULL;

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
bool GUI::sameOutputChannel = false;
int GUI::viewModeSelection = 0;
int GUI::lossSelfSelection = 0;
int GUI::lossTargetSelection = 0;
int GUI::resolutionX = 0;
int GUI::resolutionY = 0;
Vector3 GUI::waypointStartPosition;
Matrix GUI::waypointStartRotation;

GUITab* GUI::tabGroundTruth = NULL;
GUITab* GUI::tabOctree = NULL;

// Renderer
std::vector<IGUIElement*> GUI::rendererElements;
std::vector<IGUIElement*> GUI::splatElements;
std::vector<IGUIElement*> GUI::octreeElements;
std::vector<IGUIElement*> GUI::sparseElements;
std::vector<IGUIElement*> GUI::pointElements;
std::vector<IGUIElement*> GUI::pullPushElements;
std::vector<IGUIElement*> GUI::meshElements;
std::vector<IGUIElement*> GUI::neuralNetworkElements;

// Advanced
std::vector<IGUIElement*> GUI::advancedElements;

// HDF5
std::vector<IGUIElement*> GUI::datasetElements;

void PointCloudEngine::GUI::Initialize()
{
	if (!initialized)
	{
		LoadCameraRecording();

		// Place the gui window for changing settings next to the main window
		RECT rect;
		GetWindowRect(hwndScene, &rect);

		IGUIElement::InitializeFontHandle(GS(20));

		// Tab inside the gui window for choosing different groups of settings
		tabGroundTruth = new GUITab(hwndGUI, 0, 0, GS(settings->userInterfaceWidth), GS(settings->userInterfaceHeight), { L"Renderer", L"Advanced", L"Dataset" }, OnSelectTab);
		tabOctree = new GUITab(hwndGUI, 0, 0, GS(settings->userInterfaceWidth), GS(settings->userInterfaceHeight), { L"Renderer", L"Advanced" }, OnSelectTab);

		viewModeSelection = (int)settings->viewMode;

		// Load view mode
		if (settings->useOctree)
		{
			viewModeSelection = viewModeSelection % 3;
		}
		else
		{
			viewModeSelection = viewModeSelection - 3;
		}

		// Create and show content for each tab
		CreateRendererElements();
		CreateAdvancedElements();
		CreateDatasetElements();

		initialized = true;
	}

	OnSelectTab(0);
	OnSelectViewMode();
	UpdateWindow(hwndGUI);
}

void PointCloudEngine::GUI::Release()
{
	SaveCameraRecording();

	initialized = false;

	// SafeDelete all GUI elements, HWNDs are released automatically
	IGUIElement::DeleteFontHandle();
	SAFE_DELETE(tabGroundTruth);
	SAFE_DELETE(tabOctree);
	DeleteElements(rendererElements);
	DeleteElements(splatElements);
	DeleteElements(octreeElements);
	DeleteElements(sparseElements);
	DeleteElements(pointElements);
	DeleteElements(pullPushElements);
	DeleteElements(meshElements);
	DeleteElements(neuralNetworkElements);
	DeleteElements(advancedElements);
	DeleteElements(datasetElements);
}

void PointCloudEngine::GUI::Update()
{
	if (!initialized)
		return;

	tabGroundTruth->Update();
	tabOctree->Update();
	UpdateElements(rendererElements);
	UpdateElements(splatElements);
	UpdateElements(octreeElements);
	UpdateElements(sparseElements);
	UpdateElements(pointElements);
	UpdateElements(pullPushElements);
	UpdateElements(meshElements);
	UpdateElements(neuralNetworkElements);
	UpdateElements(advancedElements);
	UpdateElements(datasetElements);
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
		HandleMessageElements(rendererElements, msg, wParam, lParam);
		HandleMessageElements(splatElements, msg, wParam, lParam);
		HandleMessageElements(octreeElements, msg, wParam, lParam);
		HandleMessageElements(sparseElements, msg, wParam, lParam);
		HandleMessageElements(pointElements, msg, wParam, lParam);
		HandleMessageElements(pullPushElements, msg, wParam, lParam);
		HandleMessageElements(meshElements, msg, wParam, lParam);
		HandleMessageElements(neuralNetworkElements, msg, wParam, lParam);
		HandleMessageElements(advancedElements, msg, wParam, lParam);
		HandleMessageElements(datasetElements, msg, wParam, lParam);
	}
}

void PointCloudEngine::GUI::SetVisible(bool visible)
{
	ShowWindow(hwndGUI, visible ? SW_SHOW : SW_HIDE);
	
	if (settings->useOctree)
	{
		((GUIDropdown*)rendererElements[2])->SetSelection(min((int)settings->viewMode, 2));
		tabOctree->Show(SW_SHOW);
		tabGroundTruth->Show(SW_HIDE);
	}
	else
	{
		((GUIDropdown*)rendererElements[1])->SetSelection(max(0, min((int)settings->viewMode - 3, 4)));
		tabGroundTruth->Show(SW_SHOW);
		tabOctree->Show(SW_HIDE);
	}
}

void PointCloudEngine::GUI::SetNeuralNetworkOutputChannels(std::vector<std::wstring> outputChannels)
{
	((GUIDropdown*)neuralNetworkElements[11])->SetEntries(outputChannels);
	((GUIDropdown*)neuralNetworkElements[13])->SetEntries(outputChannels);
	((GUIDropdown*)neuralNetworkElements[15])->SetEntries(outputChannels);
}

void PointCloudEngine::GUI::SetNeuralNetworkLossSelfChannels(std::map<std::wstring, XMUINT2> renderModes)
{
	std::vector<std::wstring> selfChannels;

	for (auto it = renderModes.begin(); it != renderModes.end(); it++)
	{
		selfChannels.push_back(it->first);
	}

	((GUIDropdown*)neuralNetworkElements[5])->SetEntries(selfChannels);
}

void PointCloudEngine::GUI::SetNeuralNetworkLossTargetChannels(std::vector<std::wstring> lossTargetChannels)
{
	((GUIDropdown*)neuralNetworkElements[7])->SetEntries(lossTargetChannels);
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
		SAFE_DELETE(*it);
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

void PointCloudEngine::GUI::CreateRendererElements()
{
	rendererElements.push_back(new GUIText(hwndGUI, GS(10), GS(40), GS(100), GS(20), L"View Mode "));
	rendererElements.push_back(new GUIDropdown(hwndGUI, GS(160), GS(35), GS(180), GS(200), { L"Splats", L"Sparse Splats", L"Points", L"Sparse Points", L"Pull Push", L"Mesh", L"Neural Network"}, OnSelectViewMode, &viewModeSelection));
	rendererElements.push_back(new GUIDropdown(hwndGUI, GS(160), GS(35), GS(180), GS(200), { L"Splats", L"Octree Nodes", L"Normal Clusters" }, OnSelectViewMode, &viewModeSelection));
	rendererElements.push_back(new GUIText(hwndGUI, GS(10), GS(70), GS(150), GS(20), L"Vertex Count "));
	rendererElements.push_back(new GUIValue<UINT>(hwndGUI, GS(160), GS(70), GS(200), GS(20), &GUI::vertexCount));
	rendererElements.push_back(new GUIText(hwndGUI, GS(10), GS(100), GS(150), GS(20), L"Frames per second "));
	rendererElements.push_back(new GUIValue<UINT>(hwndGUI, GS(160), GS(100), GS(50), GS(20), &GUI::fps));
	rendererElements.push_back(new GUIText(hwndGUI, GS(10), GS(130), GS(150), GS(20), L"Lighting "));
	rendererElements.push_back(new GUICheckbox(hwndGUI, GS(160), GS(130), GS(20), GS(20), L"", NULL, &settings->useLighting));

	splatElements.push_back(new GUIText(hwndGUI, GS(10), GS(160), GS(150), GS(20), L"Blending "));
	splatElements.push_back(new GUICheckbox(hwndGUI, GS(160), GS(160), GS(20), GS(20), L"", NULL, &settings->useBlending));
	splatElements.push_back(new GUISlider<float>(hwndGUI, GS(160), GS(190), GS(130), GS(20), 0, 100, 10, 0, L"Blend Factor", &settings->blendFactor, 1, GS(148), GS(40)));
	splatElements.push_back(new GUISlider<float>(hwndGUI, GS(160), GS(220), GS(130), GS(20), 0, 1000, 1000, 0, L"Sampling Rate", &settings->samplingRate, 3, GS(148), GS(40)));

	octreeElements.push_back(new GUISlider<float>(hwndGUI, GS(160), GS(250), GS(130), GS(20), 4, 100, max(settings->resolutionX, settings->resolutionY) * 4, 0, L"Splat Resolution", &settings->splatResolution, 4, GS(148), GS(50)));
	octreeElements.push_back(new GUISlider<float>(hwndGUI, GS(160), GS(280), GS(130), GS(20), 0, 500, 100, -100, L"Overlap Factor", &settings->overlapFactor, 2, GS(148), GS(40)));
	octreeElements.push_back(new GUISlider<int>(hwndGUI, GS(160), GS(310), GS(130), GS(20), 0, 1 + (UINT)settings->maxOctreeDepth, 1, 1, L"Octree Level", &settings->octreeLevel, 1, GS(148), GS(40)));
	octreeElements.push_back(new GUIText(hwndGUI, GS(10), GS(340), GS(150), GS(20), L"Culling "));
	octreeElements.push_back(new GUICheckbox(hwndGUI, GS(160), GS(340), GS(20), GS(20), L"", NULL, &settings->useCulling));
	octreeElements.push_back(new GUIText(hwndGUI, GS(10), GS(370), GS(150), GS(20), L"GPU Traversal "));
	octreeElements.push_back(new GUICheckbox(hwndGUI, GS(160), GS(370), GS(20), GS(20), L"", NULL, &settings->useGPUTraversal));

	sparseElements.push_back(new GUISlider<float>(hwndGUI, GS(160), GS(220), GS(130), GS(20), 0, 1000, 100, 0, L"Sparse Sampling Rate", &settings->sparseSamplingRate, 2, GS(148), GS(40)));
	sparseElements.push_back(new GUISlider<float>(hwndGUI, GS(160), GS(250), GS(130), GS(20), 0, 1000, 1000, 0, L"Density", &settings->density, 3, GS(148), GS(40)));

	pointElements.push_back(new GUIText(hwndGUI, GS(10), GS(160), GS(150), GS(20), L"Backface Culling "));
	pointElements.push_back(new GUICheckbox(hwndGUI, GS(160), GS(160), GS(20), GS(20), L"", NULL, &settings->backfaceCulling));

	pullPushElements.push_back(new GUIText(hwndGUI, GS(10), GS(220), GS(150), GS(20), L"Linear Filtering "));
	pullPushElements.push_back(new GUICheckbox(hwndGUI, GS(160), GS(220), GS(20), GS(20), L"", NULL, &settings->pullPushLinearFilter));
	pullPushElements.push_back(new GUIText(hwndGUI, GS(10), GS(250), GS(150), GS(20), L"Skip Push Phase "));
	pullPushElements.push_back(new GUICheckbox(hwndGUI, GS(160), GS(250), GS(20), GS(20), L"", NULL, &settings->pullPushSkipPushPhase));
	pullPushElements.push_back(new GUIText(hwndGUI, GS(10), GS(280), GS(150), GS(20), L"Oriented Splat "));
	pullPushElements.push_back(new GUICheckbox(hwndGUI, GS(160), GS(280), GS(20), GS(20), L"", NULL, &settings->pullPushOrientedSplat));
	pullPushElements.push_back(new GUIText(hwndGUI, GS(10), GS(310), GS(150), GS(20), L"Texel Blending "));
	pullPushElements.push_back(new GUICheckbox(hwndGUI, GS(160), GS(310), GS(20), GS(20), L"", NULL, &settings->pullPushBlending));
	pullPushElements.push_back(new GUISlider<float>(hwndGUI, GS(160), GS(340), GS(130), GS(20), 0, 1000, 1000, 0, L"Blend Range", &settings->pullPushBlendRange, 2, GS(148), GS(40)));
	pullPushElements.push_back(new GUISlider<float>(hwndGUI, GS(160), GS(370), GS(130), GS(20), 0, 1000, 1000, 0, L"Splat Size", &settings->pullPushSplatSize, 2, GS(148), GS(40)));
	pullPushElements.push_back(new GUISlider<int>(hwndGUI, GS(160), GS(400), GS(130), GS(20), 0, 12, 1, 0, L"Pull Push Level", &settings->pullPushDebugLevel, 2, GS(148), GS(40)));

	meshElements.push_back(new GUIButton(hwndGUI, GS(10), GS(160), GS(325), GS(25), L"Load Mesh from .OBJ File", OnLoadMeshFromOBJFile));

	neuralNetworkElements.push_back(new GUIText(hwndGUI, GS(10), GS(160), GS(100), GS(20), L"TODO"));
}

void PointCloudEngine::GUI::CreateAdvancedElements()
{
	resolutionX = settings->resolutionX;
	resolutionY = settings->resolutionY;

	advancedElements.push_back(new GUISlider<int>(hwndGUI, GS(160), GS(40), GS(130), GS(20), 1, (uint32_t)GetSystemMetrics(SM_CXSCREEN), 1, 0, L"Resolution X", &resolutionX, 1, GS(148), GS(40), OnChangeResolution));
	advancedElements.push_back(new GUISlider<int>(hwndGUI, GS(160), GS(70), GS(130), GS(20), 1, (uint32_t)GetSystemMetrics(SM_CYSCREEN), 1, 0, L"Resolution Y", &resolutionY, 1, GS(148), GS(40), OnChangeResolution));
	advancedElements.push_back(new GUIButton(hwndGUI, GS(10), GS(100), GS(325), GS(25), L"Apply Resolution", OnApplyResolution));
	OnChangeResolution();

	advancedElements.push_back(new GUISlider<float>(hwndGUI, GS(160), GS(160), GS(130), GS(20), 1, 100, 10, 0, L"Scale", &settings->scale, 1, GS(148), GS(40)));
	advancedElements.push_back(new GUIText(hwndGUI, GS(10), GS(190), GS(150), GS(20), L"Background RGB"));
	advancedElements.push_back(new GUISlider<float>(hwndGUI, GS(160), GS(190), GS(45), GS(20), 0, 100, 100, 0, L"R", &settings->backgroundColor.x, 0, 0, 0));
	advancedElements.push_back(new GUISlider<float>(hwndGUI, GS(203), GS(190), GS(45), GS(20), 0, 100, 100, 0, L"G", &settings->backgroundColor.y, 0, 0, 0));
	advancedElements.push_back(new GUISlider<float>(hwndGUI, GS(245), GS(190), GS(45), GS(20), 0, 100, 100, 0, L"B", &settings->backgroundColor.z, 0, 0, 0));

	advancedElements.push_back(new GUIText(hwndGUI, GS(10), GS(250), GS(150), GS(20), L"Headlight "));
	advancedElements.push_back(new GUICheckbox(hwndGUI, GS(160), GS(250), GS(20), GS(20), L"", NULL, &settings->useHeadlight));
	advancedElements.push_back(new GUIText(hwndGUI, GS(10), GS(280), GS(150), GS(20), L"Light Direction XYZ"));
	advancedElements.push_back(new GUISlider<float>(hwndGUI, GS(160), GS(280), GS(45), GS(20), 0, 200, 100, 100, L"X", &settings->lightDirection.x, 0, 0, 0));
	advancedElements.push_back(new GUISlider<float>(hwndGUI, GS(203), GS(280), GS(45), GS(20), 0, 200, 100, 100, L"Y", &settings->lightDirection.y, 0, 0, 0));
	advancedElements.push_back(new GUISlider<float>(hwndGUI, GS(245), GS(280), GS(45), GS(20), 0, 200, 100, 100, L"Z", &settings->lightDirection.z, 0, 0, 0));
	advancedElements.push_back(new GUISlider<float>(hwndGUI, GS(160), GS(310), GS(130), GS(20), 0, 200, 100, 0, L"Light Ambient", &settings->ambient, 2, GS(148), GS(40)));
	advancedElements.push_back(new GUISlider<float>(hwndGUI, GS(160), GS(340), GS(130), GS(20), 0, 200, 100, 0, L"Light Diffuse", &settings->diffuse, 2, GS(148), GS(40)));
	advancedElements.push_back(new GUISlider<float>(hwndGUI, GS(160), GS(370), GS(130), GS(20), 0, 200, 100, 0, L"Light Specular", &settings->specular, 2, GS(148), GS(40)));
	advancedElements.push_back(new GUISlider<float>(hwndGUI, GS(160), GS(400), GS(130), GS(20), 0, 2000, 100, 0, L"Light Specular Exp.", &settings->specularExponent, 2, GS(148), GS(40)));
}

void PointCloudEngine::GUI::CreateDatasetElements()
{
	// Use this to select a camera position from the HDF5 file generation
	datasetElements.push_back(new GUISlider<UINT>(hwndGUI, GS(160), GS(40), GS(130), GS(20), 0, (UINT)cameraRecordingPositions.size() - 1, 1, 0, L"Camera Recording", &cameraRecording, 0, GS(148), GS(40), OnChangeCameraRecording));

	datasetElements.push_back(new GUIText(hwndGUI, GS(10), GS(80), GS(300), GS(20), L"Waypoint Dataset Generation"));
	datasetElements.push_back(new GUISlider<float>(hwndGUI, GS(160), GS(110), GS(130), GS(20), 1, 1000, 1000, 0, L"Step Size", &settings->waypointStepSize, 3, GS(148), GS(40)));
	datasetElements.push_back(new GUISlider<float>(hwndGUI, GS(160), GS(140), GS(130), GS(20), 1, 1000, 1000, 0, L"Preview Step Size", &settings->waypointPreviewStepSize, 3, GS(148), GS(40)));
	datasetElements.push_back(new GUIText(hwndGUI, GS(10), GS(170), GS(150), GS(20), L"Range Min/Max"));
	datasetElements.push_back(new GUISlider<float>(hwndGUI, GS(160), GS(170), GS(50), GS(20), 0, 100, 100, 0, L"Min", &settings->waypointMin, 1, 0, GS(28)));
	datasetElements.push_back(new GUISlider<float>(hwndGUI, GS(240), GS(170), GS(50), GS(20), 0, 100, 100, 0, L"Max", &settings->waypointMax, 1, 0, GS(28)));
	datasetElements.push_back(new GUIButton(hwndGUI, GS(10), GS(200), GS(150), GS(25), L"Add Waypoint", OnWaypointAdd));
	datasetElements.push_back(new GUIButton(hwndGUI, GS(185), GS(200), GS(150), GS(25), L"Remove Waypoint", OnWaypointRemove));
	datasetElements.push_back(new GUIButton(hwndGUI, GS(10), GS(235), GS(150), GS(25), L"Toggle Waypoints", OnWaypointToggle));
	datasetElements.push_back(new GUIButton(hwndGUI, GS(185), GS(235), GS(150), GS(25), L"Preview Waypoints", OnWaypointPreview));
	datasetElements.push_back(new GUIButton(hwndGUI, GS(10), GS(270), GS(325), GS(25), L"Generate Waypoint Dataset", OnGenerateWaypointDataset));

	datasetElements.push_back(new GUIText(hwndGUI, GS(10), GS(330), GS(300), GS(20), L"Sphere Dataset Generation"));
	datasetElements.push_back(new GUISlider<float>(hwndGUI, GS(160), GS(360), GS(130), GS(20), 1, 6283, 1000, 0, L"Sphere Step Size", &settings->sphereStepSize, 3, GS(148), GS(40)));
	datasetElements.push_back(new GUIText(hwndGUI, GS(10), GS(390), GS(150), GS(20), L"Theta Min/Max"));
	datasetElements.push_back(new GUISlider<float>(hwndGUI, GS(160), GS(390), GS(50), GS(20), 1, 628, 100, 0, L"Min", &settings->sphereMinTheta, 1, 0, GS(28)));
	datasetElements.push_back(new GUISlider<float>(hwndGUI, GS(240), GS(390), GS(50), GS(20), 1, 628, 100, 0, L"Max", &settings->sphereMaxTheta, 1, 0, GS(28)));
	datasetElements.push_back(new GUIText(hwndGUI, GS(10), GS(420), GS(150), GS(20), L"Phi Min/Max"));
	datasetElements.push_back(new GUISlider<float>(hwndGUI, GS(160), GS(420), GS(50), GS(20), 1, 628, 100, 0, L"Min", &settings->sphereMinPhi, 1, 0, GS(28)));
	datasetElements.push_back(new GUISlider<float>(hwndGUI, GS(240), GS(420), GS(50), GS(20), 1, 628, 100, 0, L"Max", &settings->sphereMaxPhi, 1, 0, GS(28)));
	datasetElements.push_back(new GUIButton(hwndGUI, GS(10), GS(450), GS(325), GS(25), L"Generate Sphere Dataset", OnGenerateSphereDataset));
}

void PointCloudEngine::GUI::LoadCameraRecording()
{
	// Load the stored camera recordings from a file
	std::ifstream file(executableDirectory + CAMERARECORDINGS_FILENAME, std::ios::in | std::ios::binary);

	if (file.is_open())
	{
		UINT cameraRecordingSize = 0;

		// Read the count
		file.read((char*)&cameraRecordingSize, sizeof(UINT));

		// Reserve memory
		cameraRecordingPositions.resize(cameraRecordingSize);
		cameraRecordingRotations.resize(cameraRecordingSize);

		// Read the positions and rotations into the vectors
		file.read((char*)cameraRecordingPositions.data(), sizeof(Vector3) * cameraRecordingSize);
		file.read((char*)cameraRecordingRotations.data(), sizeof(Matrix) * cameraRecordingSize);
	}
}

void PointCloudEngine::GUI::SaveCameraRecording()
{
	// Save camera recordings to file
	std::ofstream file(executableDirectory + CAMERARECORDINGS_FILENAME, std::ios::out | std::ios::binary);

	UINT cameraRecordingSize = cameraRecordingPositions.size();

	file.write((char*)&cameraRecordingSize, sizeof(UINT));
	file.write((char*)cameraRecordingPositions.data(), sizeof(Vector3) * cameraRecordingSize);
	file.write((char*)cameraRecordingRotations.data(), sizeof(Matrix) * cameraRecordingSize);
	file.flush();
	file.close();
}

void PointCloudEngine::GUI::OnSelectViewMode()
{
	ShowElements(octreeElements, SW_HIDE);
	ShowElements(splatElements, SW_HIDE);
	ShowElements(sparseElements, SW_HIDE);
	ShowElements(pointElements, SW_HIDE);
	ShowElements(pullPushElements, SW_HIDE);
	ShowElements(meshElements, SW_HIDE);
	ShowElements(neuralNetworkElements, SW_HIDE);

	if (settings->useOctree)
	{
		settings->viewMode = (ViewMode)(viewModeSelection % 3);

		ShowElements(splatElements);
		ShowElements(octreeElements);
	}
	else
	{
		settings->viewMode = (ViewMode)((viewModeSelection + 3) % 10);

		switch (settings->viewMode)
		{
			case ViewMode::Splats:
			{
				ShowElements(splatElements);
				break;
			}
			case ViewMode::SparseSplats:
			{
				ShowElements(splatElements);
				ShowElements(sparseElements);
				splatElements[3]->Show(SW_HIDE);
				sparseElements[1]->SetPosition(GS(160), GS(250));
				break;
			}
			case ViewMode::Points:
			{
				ShowElements(pointElements);
				break;
			}
			case ViewMode::SparsePoints:
			{
				ShowElements(pointElements);
				sparseElements[1]->Show(SW_SHOW);
				sparseElements[1]->SetPosition(GS(160), GS(190));
				break;
			}
			case ViewMode::PullPush:
			{
				ShowElements(pointElements);
				ShowElements(pullPushElements);
				sparseElements[1]->Show(SW_SHOW);
				sparseElements[1]->SetPosition(GS(160), GS(190));
				break;
			}
			case ViewMode::Mesh:
			{
				ShowElements(meshElements);
				break;
			}
			case ViewMode::NeuralNetwork:
			{
				ShowElements(neuralNetworkElements);
				break;
			}
		}
	}
}

void PointCloudEngine::GUI::OnSelectTab(int selection)
{
	ShowElements(rendererElements, SW_HIDE);
	ShowElements(splatElements, SW_HIDE);
	ShowElements(octreeElements, SW_HIDE);
	ShowElements(sparseElements, SW_HIDE);
	ShowElements(pointElements, SW_HIDE);
	ShowElements(pullPushElements, SW_HIDE);
	ShowElements(meshElements, SW_HIDE);
	ShowElements(neuralNetworkElements, SW_HIDE);
	ShowElements(advancedElements, SW_HIDE);
	ShowElements(datasetElements, SW_HIDE);

	switch (selection)
	{
		case 0:
		{
			ShowElements(rendererElements);
			OnSelectViewMode();

			if (settings->useOctree)
			{
				rendererElements[1]->Show(SW_HIDE);
			}
			else
			{
				rendererElements[2]->Show(SW_HIDE);
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
			ShowElements(datasetElements);
			break;
		}
	}
}

void PointCloudEngine::GUI::OnChangeResolution()
{
	std::wstring buttonText = L"Apply Resolution " + std::to_wstring(resolutionX) + L"x" + std::to_wstring(resolutionY);
	SetWindowText(((GUIButton*)advancedElements[2])->hwndElement, buttonText.c_str());
}

void PointCloudEngine::GUI::OnApplyResolution()
{	
	ChangeRenderingResolution(resolutionX, resolutionY);

#ifndef IGNORE_OLD_PYTORCH_AND_HDF5_IMPLEMENTATION
	// Make sure the neural network renderer reallocates resources as well
	if (groundTruthRenderer != NULL)
	{
		groundTruthRenderer->ApplyNeuralNetworkResolution();
	}
#endif


	// Octree maximal splat resolution should increase/decrease when changing the resolution
	((GUISlider<float>*)octreeElements[0])->scale = max(settings->resolutionX, settings->resolutionY) * 4;
}

void PointCloudEngine::GUI::OnLoadMeshFromOBJFile()
{
	std::cout << "TODO" << std::endl;
}

void PointCloudEngine::GUI::OnWaypointAdd()
{
	if (waypointRenderer != NULL)
	{
		waypointRenderer->enabled = true;
		waypointRenderer->AddWaypoint(camera->GetPosition(), camera->GetRotationMatrix(), camera->GetForward());
	}
}

void PointCloudEngine::GUI::OnWaypointRemove()
{
	if (waypointRenderer != NULL)
	{
		waypointRenderer->enabled = true;
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
		waypointRenderer->enabled = true;
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
#ifndef IGNORE_OLD_PYTORCH_AND_HDF5_IMPLEMENTATION
	if (groundTruthRenderer != NULL)
	{
		groundTruthRenderer->GenerateWaypointDataset();
	}
#endif
	((GUISlider<UINT>*)datasetElements[0])->SetRange(0, cameraRecordingPositions.size() - 1);
}

void PointCloudEngine::GUI::OnGenerateSphereDataset()
{
#ifndef IGNORE_OLD_PYTORCH_AND_HDF5_IMPLEMENTATION
	if (groundTruthRenderer != NULL)
	{
		groundTruthRenderer->GenerateSphereDataset();
	}
#endif
	((GUISlider<UINT>*)datasetElements[0])->SetRange(0, cameraRecordingPositions.size() - 1);
}

void PointCloudEngine::GUI::OnChangeCameraRecording()
{
	camera->SetPosition(cameraRecordingPositions[cameraRecording]);
	camera->SetRotationMatrix(cameraRecordingRotations[cameraRecording]);
}

void PointCloudEngine::GUI::OnSelectNeuralNetworkLossFunction()
{
	if (lossFunctionSelection == 1)
	{
		((GUIValue<float>*)neuralNetworkElements[2])->value = &l1Loss;
	}
	else if (lossFunctionSelection == 2)
	{
		((GUIValue<float>*)neuralNetworkElements[2])->value = &mseLoss;
	}
	else if (lossFunctionSelection == 3)
	{
		((GUIValue<float>*)neuralNetworkElements[2])->value = &smoothL1Loss;
	}
	else
	{
		((GUIValue<float>*)neuralNetworkElements[2])->value = NULL;
	}
}

void PointCloudEngine::GUI::OnSelectNeuralNetworkLossSelf()
{
	settings->lossCalculationSelf = ((GUIDropdown*)neuralNetworkElements[5])->GetSelectedString();
}

void PointCloudEngine::GUI::OnSelectNeuralNetworkLossTarget()
{
	settings->lossCalculationTarget = ((GUIDropdown*)neuralNetworkElements[7])->GetSelectedString();
}

void PointCloudEngine::GUI::OnSelectNeuralNetworkOutputRed()
{
	if (sameOutputChannel)
	{
		((GUIDropdown*)neuralNetworkElements[13])->SetSelection(settings->neuralNetworkOutputRed);
		((GUIDropdown*)neuralNetworkElements[15])->SetSelection(settings->neuralNetworkOutputRed);
	}
}

void PointCloudEngine::GUI::OnSelectNeuralNetworkOutputGreen()
{
	if (sameOutputChannel)
	{
		((GUIDropdown*)neuralNetworkElements[11])->SetSelection(settings->neuralNetworkOutputGreen);
		((GUIDropdown*)neuralNetworkElements[15])->SetSelection(settings->neuralNetworkOutputGreen);
	}
}

void PointCloudEngine::GUI::OnSelectNeuralNetworkOutputBlue()
{
	if (sameOutputChannel)
	{
		((GUIDropdown*)neuralNetworkElements[11])->SetSelection(settings->neuralNetworkOutputBlue);
		((GUIDropdown*)neuralNetworkElements[13])->SetSelection(settings->neuralNetworkOutputBlue);
	}
}

void PointCloudEngine::GUI::OnLoadPytorchModel()
{
#ifndef IGNORE_OLD_PYTORCH_AND_HDF5_IMPLEMENTATION
	if (OpenFileDialog(L"Pytorch Jit Model Files\0*.pt\0\0", settings->neuralNetworkModelFile))
	{
		groundTruthRenderer->LoadNeuralNetworkPytorchModel();
	}
#endif
}

void PointCloudEngine::GUI::OnLoadDescriptionFile()
{
#ifndef IGNORE_OLD_PYTORCH_AND_HDF5_IMPLEMENTATION
	if (OpenFileDialog(L"Model Description Files\0*.txt\0\0", settings->neuralNetworkDescriptionFile))
	{
		groundTruthRenderer->LoadNeuralNetworkDescriptionFile();
	}
#endif
}
