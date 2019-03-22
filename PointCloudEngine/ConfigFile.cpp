#include "ConfigFile.h"

#define CONFIGFILENAME L"Assets/.pointcloudengine"

/*
    The file just consists of two lines with:
    1. string plyfile
    2. float scale
*/

PointCloudEngine::ConfigFile::ConfigFile()
{
    // Check if the config file exists that stores the last plyfile path
    std::wifstream configFile(CONFIGFILENAME);

    if (configFile.is_open())
    {
        // Parse the data
        std::wstring line;
        std::getline(configFile, plyfile);
        std::getline(configFile, line);
        scale = std::stof(line);
    }
}

PointCloudEngine::ConfigFile::~ConfigFile()
{
    // Save values to file
    std::wofstream configFile(CONFIGFILENAME);
    configFile << plyfile << std::endl << scale;
    configFile.flush();
    configFile.close();
}
