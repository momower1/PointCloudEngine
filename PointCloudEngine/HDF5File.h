#ifndef HDF5FILE_H
#define HDF5FILE_H

#ifndef IGNORE_OLD_PYTORCH_AND_HDF5_IMPLEMENTATION

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

	void AddColorTextureDataset(H5::Group& group, std::wstring name, ID3D11Texture2D* texture, bool sparseUpsample = false, float gammaCorrection = 1.0f);
	void AddColorTextureDataset(H5::Group& group, std::string name, ID3D11Texture2D* texture, bool sparseUpsample = false, float gammaCorrection = 1.0f);
	void AddDepthTextureDataset(H5::Group& group, std::wstring name, ID3D11Texture2D* texture, bool sparseUpsample = false);
	void AddDepthTextureDataset(H5::Group& group, std::string name, ID3D11Texture2D* texture, bool sparseUpsample = false);
	void AddStringAttribute(std::wstring name, std::wstring value);

private:
	H5::H5File* file = NULL;

	void AddSparseUpsampleOfColorTexture(H5::Group& group, std::string name, int width, int height, std::vector<byte> data);
	void AddSparseUpsampleOfDepthTexture(H5::Group& group, std::string name, int width, int height, float* data);
	H5::DataSpace CreateDataspace(std::initializer_list<hsize_t> dimensions = {});
	H5::DSetCreatPropList CreateDeflateCompressionPropList(std::initializer_list<hsize_t> chunkDimensions = {}, int deflateLevel = 6);
	void AddStringAttribute(H5::H5Object* object, std::wstring name, std::wstring value);
	void AddStringAttribute(H5::H5Object* object, std::string name, std::string value);
	void SetImageAttributes(H5::DataSet& dataSet);
};
#endif
#endif