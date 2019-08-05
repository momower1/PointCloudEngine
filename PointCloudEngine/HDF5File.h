#ifndef HDF5FILE_H
#define HDF5FILE_H

#define H5_BUILT_AS_DYNAMIC_LIB

#pragma once
#include "hdf5.h"
#include "PointCloudEngine.h"

class HDF5File
{
public:
	HDF5File(std::wstring filename);
	HDF5File(std::string filename);
	~HDF5File();

	void SaveTexture2DToDataset(std::wstring name, ID3D11Texture2D* texture);
	void SaveTexture2DToDataset(std::string name, ID3D11Texture2D* texture);

private:
	hid_t fileID;
	herr_t status;
};

#endif