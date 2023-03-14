#include "Settings.h"

PointCloudEngine::Settings::Settings(std::wstring filename)
{
	this->filename = filename;

    // Check if the file exists (otherwise use default values)
    std::wifstream settingsFile(filename);

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

		// Parse engine window parameters
		TryParse(NAMEOF(guiScaleFactor), &guiScaleFactor);
		TryParse(NAMEOF(engineWidth), &engineWidth);
		TryParse(NAMEOF(engineHeight), &engineHeight);
		TryParse(NAMEOF(enginePositionX), &enginePositionX);
		TryParse(NAMEOF(enginePositionY), &enginePositionY);
		TryParse(NAMEOF(showUserInterface), &showUserInterface);

		// Parse rendering parameters
		TryParse(NAMEOF(backgroundColor), &backgroundColor);
		TryParse(NAMEOF(fovAngleY), &fovAngleY);
		TryParse(NAMEOF(nearZ), &nearZ);
		TryParse(NAMEOF(farZ), &farZ);
		TryParse(NAMEOF(resolutionX), &resolutionX);
		TryParse(NAMEOF(resolutionY), &resolutionY);
		TryParse(NAMEOF(windowed), &windowed);
		TryParse(NAMEOF(viewMode), &viewMode);
		TryParse(NAMEOF(shadingMode), &shadingMode);

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

		// Parse pull push parameters
		TryParse(NAMEOF(pullPushLinearFilter), &pullPushLinearFilter);
		TryParse(NAMEOF(pullPushSkipPushPhase), &pullPushSkipPushPhase);
		TryParse(NAMEOF(pullPushOrientedSplat), &pullPushOrientedSplat);
		TryParse(NAMEOF(pullPushBlending), &pullPushBlending);
		TryParse(NAMEOF(pullPushDebugLevel), &pullPushDebugLevel);
		TryParse(NAMEOF(pullPushSplatSize), &pullPushSplatSize);
		TryParse(NAMEOF(pullPushBlendRange), &pullPushBlendRange);

		// Mesh parameters
		TryParse(NAMEOF(meshFile), &meshFile);
		TryParse(NAMEOF(loadMeshFile), &loadMeshFile);
		TryParse(NAMEOF(textureLOD), &textureLOD);

		// Parse neural network parameters
		TryParse(NAMEOF(useCUDA), &useCUDA);
		TryParse(NAMEOF(thresholdSCM), &thresholdSCM);
		TryParse(NAMEOF(thresholdSurfaceKeeping), &thresholdSurfaceKeeping);
		TryParse(NAMEOF(filenameSCM), &filenameSCM);
		TryParse(NAMEOF(filenameSFM), &filenameSFM);
		TryParse(NAMEOF(filenameSRM), &filenameSRM);

		// Parse HDF5 dataset generation parameters
		TryParse(NAMEOF(compressDataset), &compressDataset);
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
    std::wofstream settingsFile(this->filename);
	settingsFile << ToKeyValueString();
    settingsFile.flush();
    settingsFile.close();
}

std::wstring PointCloudEngine::Settings::ToKeyValueString()
{
	std::wstringstream settingsStream;

	settingsStream << L"# Parameters only apply when restarting the engine." << std::endl;
	settingsStream << L"# This file is overwritten when PointCloudEngine exits." << std::endl;
	settingsStream << L"# Make sure to close the engine before editing this file." << std::endl;
	settingsStream << std::endl;

	settingsStream << L"# Engine Window Parameters" << std::endl;
	settingsStream << NAMEOF(guiScaleFactor) << L"=" << guiScaleFactor << std::endl;
	settingsStream << NAMEOF(engineWidth) << L"=" << engineWidth << std::endl;
	settingsStream << NAMEOF(engineHeight) << L"=" << engineHeight << std::endl;
	settingsStream << NAMEOF(enginePositionX) << L"=" << enginePositionX << std::endl;
	settingsStream << NAMEOF(enginePositionY) << L"=" << enginePositionY << std::endl;
	settingsStream << NAMEOF(showUserInterface) << L"=" << showUserInterface << std::endl;
	settingsStream << std::endl;

	settingsStream << L"# Rendering Parameters" << std::endl;
	settingsStream << NAMEOF(viewMode) << L"=" << (int)viewMode << std::endl;
	settingsStream << NAMEOF(shadingMode) << L"=" << (int)shadingMode << std::endl;
	settingsStream << NAMEOF(backgroundColor) << L"=" << ToString(backgroundColor) << std::endl;
	settingsStream << NAMEOF(fovAngleY) << L"=" << fovAngleY << std::endl;
	settingsStream << NAMEOF(nearZ) << L"=" << nearZ << std::endl;
	settingsStream << NAMEOF(farZ) << L"=" << farZ << std::endl;
	settingsStream << NAMEOF(resolutionX) << L"=" << resolutionX << std::endl;
	settingsStream << NAMEOF(resolutionY) << L"=" << resolutionY << std::endl;
	settingsStream << NAMEOF(windowed) << L"=" << windowed << std::endl;
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

	settingsStream << L"# Pull Push Parameters" << std::endl;
	settingsStream << NAMEOF(pullPushLinearFilter) << L"=" << pullPushLinearFilter << std::endl;
	settingsStream << NAMEOF(pullPushSkipPushPhase) << L"=" << pullPushSkipPushPhase << std::endl;
	settingsStream << NAMEOF(pullPushOrientedSplat) << L"=" << pullPushOrientedSplat << std::endl;
	settingsStream << NAMEOF(pullPushBlending) << L"=" << pullPushBlending << std::endl;
	settingsStream << NAMEOF(pullPushDebugLevel) << L"=" << pullPushDebugLevel << std::endl;
	settingsStream << NAMEOF(pullPushSplatSize) << L"=" << pullPushSplatSize << std::endl;
	settingsStream << NAMEOF(pullPushBlendRange) << L"=" << pullPushBlendRange << std::endl;
	settingsStream << std::endl;

	settingsStream << L"# Mesh Parameters" << std::endl;
	settingsStream << NAMEOF(meshFile) << L"=" << meshFile << std::endl;
	settingsStream << NAMEOF(loadMeshFile) << L"=" << loadMeshFile << std::endl;
	settingsStream << NAMEOF(textureLOD) << L"=" << textureLOD << std::endl;
	settingsStream << std::endl;

	settingsStream << L"# Neural Network Parameters" << std::endl;
	settingsStream << NAMEOF(useCUDA) << L"=" << useCUDA << std::endl;
	settingsStream << NAMEOF(thresholdSCM) << L"=" << thresholdSCM << std::endl;
	settingsStream << NAMEOF(thresholdSurfaceKeeping) << L"=" << thresholdSurfaceKeeping << std::endl;
	settingsStream << NAMEOF(filenameSCM) << L"=" << filenameSCM << std::endl;
	settingsStream << NAMEOF(filenameSFM) << L"=" << filenameSFM << std::endl;
	settingsStream << NAMEOF(filenameSRM) << L"=" << filenameSRM << std::endl;
	settingsStream << std::endl;

	settingsStream << L"# Dataset Generation Parameters" << std::endl;
	settingsStream << NAMEOF(compressDataset) << L"=" << compressDataset << std::endl;
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

std::wstring PointCloudEngine::Settings::GetFilename()
{
	return this->filename;
}
