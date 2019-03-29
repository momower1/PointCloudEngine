#include "Settings.h"

#define SETTINGS_FILENAME L"/settings.pointcloudengine"

PointCloudEngine::Settings::Settings()
{
    // Check if the config file exists that stores the last plyfile path
    std::wifstream configFile(executableDirectory + SETTINGS_FILENAME);

    if (configFile.is_open())
    {
        std::wstring line;

        while (std::getline(configFile, line))
        {
            // Split the string
            int delimiter = line.find(L"=");

            if (delimiter > 0)
            {
                std::wstring variableNameW = line.substr(0, delimiter);
                std::wstring variableValue = line.substr(delimiter + 1, line.length());

                // nameof doesn't work with wstring sadly, convert to string
                std::string variableName = std::string(variableNameW.begin(), variableNameW.end());

                // Parse by variable name
                if (variableName.compare(nameof(fovAngleY)) == 0)
                {
                    fovAngleY = std::stof(variableValue);
                }

                if (variableName.compare(nameof(resolutionX)) == 0)
                {
                    resolutionX = std::stoi(variableValue);
                }

                if (variableName.compare(nameof(resolutionY)) == 0)
                {
                    resolutionY = std::stoi(variableValue);
                }

                if (variableName.compare(nameof(msaaCount)) == 0)
                {
                    msaaCount = std::stoi(variableValue);
                }

                if (variableName.compare(nameof(windowed)) == 0)
                {
                    windowed = std::stoi(variableValue);
                }

                if (variableName.compare(nameof(plyfile)) == 0)
                {
                    plyfile = variableValue;
                }

                if (variableName.compare(nameof(maxOctreeDepth)) == 0)
                {
                    maxOctreeDepth = std::stoi(variableValue);
                }

                if (variableName.compare(nameof(scale)) == 0)
                {
                    scale = std::stof(variableValue);
                }

                if (variableName.compare(nameof(mouseSensitivity)) == 0)
                {
                    mouseSensitivity = std::stof(variableValue);
                }

                if (variableName.compare(nameof(scrollSensitivity)) == 0)
                {
                    scrollSensitivity = std::stof(variableValue);
                }
            }
        }
    }
}

PointCloudEngine::Settings::~Settings()
{
    // Save values as lines with "variableName=variableValue" to file with comments
    std::wofstream configFile(executableDirectory + SETTINGS_FILENAME);

    configFile << L"# Rendering Parameters" << std::endl;
    configFile << nameof(fovAngleY) << L"=" << fovAngleY << std::endl;
    configFile << nameof(resolutionX) << L"=" << resolutionX << std::endl;
    configFile << nameof(resolutionY) << L"=" << resolutionY << std::endl;
    configFile << nameof(msaaCount) << L"=" << msaaCount << std::endl;
    configFile << nameof(windowed) << L"=" << windowed << std::endl;
    configFile << std::endl;

    configFile << L"# Ply File Parameters" << std::endl;
    configFile << nameof(plyfile) << L"=" << plyfile << std::endl;
    configFile << nameof(maxOctreeDepth) << L"=" << maxOctreeDepth << std::endl;
    configFile << nameof(scale) << L"=" << scale << std::endl;
    configFile << std::endl;

    configFile << L"# Input Parameters" << std::endl;
    configFile << nameof(mouseSensitivity) << L"=" << mouseSensitivity << std::endl;
    configFile << nameof(scrollSensitivity) << L"=" << scrollSensitivity << std::endl;
    configFile << std::endl;

    configFile.flush();
    configFile.close();
}
