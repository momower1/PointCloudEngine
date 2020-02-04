#include "Settings.h"

#define SETTINGS_FILENAME L"/Settings.txt"

PointCloudEngine::Settings::Settings()
{
    // Check if the config file exists that stores the last pointcloudFile path
    std::wifstream settingsFile(executableDirectory + SETTINGS_FILENAME);

    if (settingsFile.is_open())
    {
        std::wstring line;

        while (std::getline(settingsFile, line))
        {
            // Split the string
            size_t delimiter = line.find(L"=");

            if (delimiter > 0)
            {
                std::wstring variableName = line.substr(0, delimiter);
                std::wstring variableValue = line.substr(delimiter + 1, line.length());

				// Put both into the map
				settingsMap[variableName] = variableValue;
            }
        }

		// Parse rendering parameters
		TryParse(NAMEOF(backgroundColor), &backgroundColor);
		TryParse(NAMEOF(fovAngleY), &fovAngleY);
		TryParse(NAMEOF(nearZ), &nearZ);
		TryParse(NAMEOF(farZ), &farZ);
		TryParse(NAMEOF(resolutionX), &resolutionX);
		TryParse(NAMEOF(resolutionY), &resolutionY);
		TryParse(NAMEOF(windowed), &windowed);
		TryParse(NAMEOF(help), &help);
		TryParse(NAMEOF(viewMode), &viewMode);

		// Parse pointcloud file parameters
		TryParse(NAMEOF(pointcloudFile), &pointcloudFile);
		TryParse(NAMEOF(samplingRate), &samplingRate);
		TryParse(NAMEOF(scale), &scale);

		// Parse lighting parameters
		TryParse(NAMEOF(useLighting), &useLighting);
		TryParse(NAMEOF(useHeadlight), &useHeadlight);
		TryParse(NAMEOF(lightDirection), &lightDirection);
		TryParse(NAMEOF(lightIntensity), &lightIntensity);
		TryParse(NAMEOF(ambient), &ambient);
		TryParse(NAMEOF(diffuse), &diffuse);
		TryParse(NAMEOF(specular), &specular);
		TryParse(NAMEOF(specularExponent), &specularExponent);

		// Parse blending parameters
		TryParse(NAMEOF(useBlending), &useBlending);
		TryParse(NAMEOF(blendFactor), &blendFactor);

		// Parse ground truth parameters
		TryParse(NAMEOF(backfaceCulling), &backfaceCulling);
		TryParse(NAMEOF(density), &density);
		TryParse(NAMEOF(sparseSamplingRate), &sparseSamplingRate);

		// Parse neural network parameters
		TryParse(NAMEOF(neuralNetworkModelFile), &neuralNetworkModelFile);
		TryParse(NAMEOF(neuralNetworkDescriptionFile), &neuralNetworkDescriptionFile);
		TryParse(NAMEOF(useCUDA), &useCUDA);
		TryParse(NAMEOF(neuralNetworkLossArea), &neuralNetworkLossArea);
		TryParse(NAMEOF(neuralNetworkOutputRed), &neuralNetworkOutputRed);
		TryParse(NAMEOF(neuralNetworkOutputGreen), &neuralNetworkOutputGreen);
		TryParse(NAMEOF(neuralNetworkOutputBlue), &neuralNetworkOutputBlue);
		TryParse(NAMEOF(lossCalculationSelf), &lossCalculationSelf);
		TryParse(NAMEOF(lossCalculationTarget), &lossCalculationTarget);

		// Parse HDF5 dataset generation parameters
		TryParse(NAMEOF(waypointStepSize), &waypointStepSize);
		TryParse(NAMEOF(waypointPreviewStepSize), &waypointPreviewStepSize);
		TryParse(NAMEOF(waypointMin), &waypointMin);
		TryParse(NAMEOF(waypointMax), &waypointMax);
		TryParse(NAMEOF(sphereStepSize), &sphereStepSize);
		TryParse(NAMEOF(sphereMinTheta), &sphereMinTheta);
		TryParse(NAMEOF(sphereMaxTheta), &sphereMaxTheta);
		TryParse(NAMEOF(sphereMinPhi), &sphereMinPhi);
		TryParse(NAMEOF(sphereMaxPhi), &sphereMaxPhi);

		// Parse octree parameters
		TryParse(NAMEOF(useOctree), &useOctree);
		TryParse(NAMEOF(useCulling), &useCulling);
		TryParse(NAMEOF(useGPUTraversal), &useGPUTraversal);
		TryParse(NAMEOF(maxOctreeDepth), &maxOctreeDepth);
		TryParse(NAMEOF(overlapFactor), &overlapFactor);
		TryParse(NAMEOF(splatResolution), &splatResolution);
		TryParse(NAMEOF(appendBufferCount), &appendBufferCount);
		TryParse(NAMEOF(octreeLevel), &octreeLevel);

		// Parse input parameters
		TryParse(NAMEOF(mouseSensitivity), &mouseSensitivity);
		TryParse(NAMEOF(scrollSensitivity), &scrollSensitivity);

		// Background color alpha has to be 0 for blending to work correctly
		backgroundColor.w = 0;
    }
}

PointCloudEngine::Settings::~Settings()
{
    // Save values as lines with "variableKey=variableValue" to file with comments
    std::wofstream settingsFile(executableDirectory + SETTINGS_FILENAME);

	settingsFile << ToKeyValueString();
    settingsFile.flush();
    settingsFile.close();
}

std::wstring PointCloudEngine::Settings::ToKeyValueString()
{
	std::wstringstream settingsStream;

	settingsStream << L"# Parameters only apply when restarting the engine!" << std::endl;
	settingsStream << std::endl;

	settingsStream << L"# Rendering Parameters" << std::endl;
	settingsStream << NAMEOF(viewMode) << L"=" << (int)viewMode << std::endl;
	settingsStream << NAMEOF(backgroundColor) << L"=" << ToString(backgroundColor) << std::endl;
	settingsStream << NAMEOF(fovAngleY) << L"=" << fovAngleY << std::endl;
	settingsStream << NAMEOF(nearZ) << L"=" << nearZ << std::endl;
	settingsStream << NAMEOF(farZ) << L"=" << farZ << std::endl;
	settingsStream << NAMEOF(resolutionX) << L"=" << resolutionX << std::endl;
	settingsStream << NAMEOF(resolutionY) << L"=" << resolutionY << std::endl;
	settingsStream << NAMEOF(windowed) << L"=" << windowed << std::endl;
	settingsStream << NAMEOF(help) << L"=" << help << std::endl;
	settingsStream << std::endl;

	settingsStream << L"# Pointcloud File Parameters" << std::endl;
	settingsStream << L"# Delete old .octree files when changing the " << NAMEOF(maxOctreeDepth) << std::endl;
	settingsStream << NAMEOF(pointcloudFile) << L"=" << pointcloudFile << std::endl;
	settingsStream << NAMEOF(samplingRate) << L"=" << samplingRate << std::endl;
	settingsStream << NAMEOF(scale) << L"=" << scale << std::endl;
	settingsStream << std::endl;

	settingsStream << L"# Lighting Parameters" << std::endl;
	settingsStream << NAMEOF(useLighting) << L"=" << useLighting << std::endl;
	settingsStream << NAMEOF(useHeadlight) << L"=" << useHeadlight << std::endl;
	settingsStream << NAMEOF(lightDirection) << L"=" << ToString(lightDirection) << std::endl;
	settingsStream << NAMEOF(lightIntensity) << L"=" << lightIntensity << std::endl;
	settingsStream << NAMEOF(ambient) << L"=" << ambient << std::endl;
	settingsStream << NAMEOF(diffuse) << L"=" << diffuse << std::endl;
	settingsStream << NAMEOF(specular) << L"=" << specular << std::endl;
	settingsStream << NAMEOF(specularExponent) << L"=" << specularExponent << std::endl;
	settingsStream << std::endl;

	settingsStream << L"# Blending Parameters" << std::endl;
	settingsStream << NAMEOF(useBlending) << L"=" << useBlending << std::endl;
	settingsStream << NAMEOF(blendFactor) << L"=" << blendFactor << std::endl;
	settingsStream << std::endl;

	settingsStream << L"# Ground Truth Parameters" << std::endl;
	settingsStream << NAMEOF(backfaceCulling) << L"=" << backfaceCulling << std::endl;
	settingsStream << NAMEOF(density) << L"=" << density << std::endl;
	settingsStream << NAMEOF(sparseSamplingRate) << L"=" << sparseSamplingRate << std::endl;
	settingsStream << std::endl;

	settingsStream << L"# Neural Network Parameters" << std::endl;
	settingsStream << NAMEOF(neuralNetworkModelFile) << L"=" << neuralNetworkModelFile << std::endl;
	settingsStream << NAMEOF(neuralNetworkDescriptionFile) << L"=" << neuralNetworkDescriptionFile << std::endl;
	settingsStream << NAMEOF(useCUDA) << L"=" << useCUDA << std::endl;
	settingsStream << NAMEOF(neuralNetworkLossArea) << L"=" << neuralNetworkLossArea << std::endl;
	settingsStream << NAMEOF(neuralNetworkOutputRed) << L"=" << neuralNetworkOutputRed << std::endl;
	settingsStream << NAMEOF(neuralNetworkOutputGreen) << L"=" << neuralNetworkOutputGreen << std::endl;
	settingsStream << NAMEOF(neuralNetworkOutputBlue) << L"=" << neuralNetworkOutputBlue << std::endl;
	settingsStream << NAMEOF(lossCalculationSelf) << L"=" << lossCalculationSelf << std::endl;
	settingsStream << NAMEOF(lossCalculationTarget) << L"=" << lossCalculationTarget << std::endl;
	settingsStream << std::endl;

	settingsStream << L"# HDF5 Dataset Generation Parameters" << std::endl;
	settingsStream << NAMEOF(waypointStepSize) << L"=" << waypointStepSize << std::endl;
	settingsStream << NAMEOF(waypointPreviewStepSize) << L"=" << waypointPreviewStepSize << std::endl;
	settingsStream << NAMEOF(waypointMin) << L"=" << waypointMin << std::endl;
	settingsStream << NAMEOF(waypointMax) << L"=" << waypointMax << std::endl;
	settingsStream << NAMEOF(sphereStepSize) << L"=" << sphereStepSize << std::endl;
	settingsStream << NAMEOF(sphereMinTheta) << L"=" << sphereMinTheta << std::endl;
	settingsStream << NAMEOF(sphereMaxTheta) << L"=" << sphereMaxTheta << std::endl;
	settingsStream << NAMEOF(sphereMinPhi) << L"=" << sphereMinPhi << std::endl;
	settingsStream << NAMEOF(sphereMaxPhi) << L"=" << sphereMaxPhi << std::endl;
	settingsStream << std::endl;

	settingsStream << L"# Octree Parameters, increase " << NAMEOF(appendBufferCount) << L" when you see flickering" << std::endl;
	settingsStream << NAMEOF(useOctree) << L"=" << useOctree << std::endl;
	settingsStream << NAMEOF(useCulling) << L"=" << useCulling << std::endl;
	settingsStream << NAMEOF(useGPUTraversal) << L"=" << useGPUTraversal << std::endl;
	settingsStream << NAMEOF(maxOctreeDepth) << L"=" << maxOctreeDepth << std::endl;
	settingsStream << NAMEOF(overlapFactor) << L"=" << overlapFactor << std::endl;
	settingsStream << NAMEOF(splatResolution) << L"=" << splatResolution << std::endl;
	settingsStream << NAMEOF(appendBufferCount) << L"=" << appendBufferCount << std::endl;
	settingsStream << NAMEOF(octreeLevel) << L"=" << octreeLevel << std::endl;
	settingsStream << std::endl;

	settingsStream << L"# Input Parameters" << std::endl;
	settingsStream << NAMEOF(mouseSensitivity) << L"=" << mouseSensitivity << std::endl;
	settingsStream << NAMEOF(scrollSensitivity) << L"=" << scrollSensitivity << std::endl;
	settingsStream << std::endl;

	return settingsStream.str();
}

std::wstring PointCloudEngine::Settings::ToString(Vector3 v)
{
	return L"(" + std::to_wstring(v.x) + L"," + std::to_wstring(v.y) + L"," + std::to_wstring(v.z) + L")";
}

std::wstring PointCloudEngine::Settings::ToString(Vector4 v)
{
	return L"(" + std::to_wstring(v.x) + L"," + std::to_wstring(v.y) + L"," + std::to_wstring(v.z) + L"," + std::to_wstring(v.w) + L")";
}

Vector3 PointCloudEngine::Settings::ToVector3(std::wstring s)
{
	// Remove brackets
	s = s.substr(1, s.length() - 1);

	// Parse the string into seperate x,y,z strings
	std::wstring x = s.substr(0, s.find(L','));
	s = s.substr(x.length() + 1, s.length());
	std::wstring y = s.substr(0, s.find(L','));
	std::wstring z = s.substr(y.length() + 1, s.length());
	
	return Vector3(std::stof(x), std::stof(y), std::stof(z));
}

Vector4 PointCloudEngine::Settings::ToVector4(std::wstring s)
{
	// Remove brackets
	s = s.substr(1, s.length() - 1);

	// Parse the string into seperate x,y,z,w strings
	std::wstring x = s.substr(0, s.find(L','));
	s = s.substr(x.length() + 1, s.length());
	std::wstring y = s.substr(0, s.find(L','));
	s = s.substr(y.length() + 1, s.length());
	std::wstring z = s.substr(0, s.find(L','));
	std::wstring w = s.substr(z.length() + 1, s.length());

	return Vector4(std::stof(x), std::stof(y), std::stof(z), std::stof(w));
}
