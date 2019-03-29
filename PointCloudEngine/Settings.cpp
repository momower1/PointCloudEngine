#include "Settings.h"

#define SETTINGS_FILENAME L"/settings.pointcloudengine"

/*
    The file just consists of two lines with:
    1. string plyfile
    2. float scale
*/

PointCloudEngine::Settings::Settings()
{
    // Check if the config file exists that stores the last plyfile path
    std::wifstream configFile(executableDirectory + SETTINGS_FILENAME);

    if (configFile.is_open())
    {
        // Parse the data
        std::wstring line;
        std::getline(configFile, plyfile);
        std::getline(configFile, line);
        scale = std::stof(line);
    }
}

PointCloudEngine::Settings::~Settings()
{
    // Save values to file
    std::wofstream configFile(executableDirectory + SETTINGS_FILENAME);
    configFile << plyfile << std::endl << scale;
    configFile.flush();
    configFile.close();
}
