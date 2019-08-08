#ifndef HDF5FILE_H
#define HDF5FILE_H

#define H5_BUILT_AS_DYNAMIC_LIB

#pragma once
#include "H5Cpp.h"
#include "PointCloudEngine.h"

class HDF5File
{
public:
	HDF5File(std::wstring filename);
	HDF5File(std::string filename);
	~HDF5File();

	H5::Group CreateGroup(std::wstring name);
	H5::Group CreateGroup(std::string name);

	void AddColorTextureDataset(H5::Group group, std::wstring name, ID3D11Texture2D* texture, float gammaCorrection = 1.0f);
	void AddColorTextureDataset(H5::Group group, std::string name, ID3D11Texture2D* texture, float gammaCorrection = 1.0f);
	void AddDepthTextureDataset(H5::Group group, std::wstring name, ID3D11Texture2D* texture);
	void AddDepthTextureDataset(H5::Group group, std::string name, ID3D11Texture2D* texture);

private:
	H5::H5File* file = NULL;

	H5::DataSpace CreateDataspace(std::initializer_list<hsize_t> dimensions = {});
	H5::DSetCreatPropList CreateDeflateCompressionPropList(std::initializer_list<hsize_t> chunkDimensions = {}, int deflateLevel = 6);
	void SetImageAttributes(H5::DataSet dataSet);
};

#endif