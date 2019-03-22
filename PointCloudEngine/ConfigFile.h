#ifndef CONFIGFILE_H
#define CONFIGFILE_H

#pragma once
#include "PointCloudEngine.h"

namespace PointCloudEngine
{
    class ConfigFile
    {
    public:
        ConfigFile();
        ~ConfigFile();

        std::wstring plyfile;
        float scale = 1.0f;
    };
}

#endif
