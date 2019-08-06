#include "HDF5File.h"

HDF5File::HDF5File(std::wstring filename)
{
	fileID = H5Fcreate(std::string(filename.begin(), filename.end()).c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
	status = 0;
}

HDF5File::HDF5File(std::string filename)
{
	fileID = H5Fcreate(filename.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
	status = 0;
}

HDF5File::~HDF5File()
{
	status = H5Fclose(fileID);
}

void HDF5File::SaveTexture2DToDataset(std::wstring name, ID3D11Texture2D* texture)
{
	SaveTexture2DToDataset(std::string(name.begin(), name.end()), texture);
}

void HDF5File::SaveTexture2DToDataset(std::string name, ID3D11Texture2D* texture)
{
	ID3D11Texture2D* tmpTexture = NULL;

	// Get the texture description
	D3D11_TEXTURE2D_DESC tmpTextureDesc;
	texture->GetDesc(&tmpTextureDesc);

	if (tmpTextureDesc.Format != DXGI_FORMAT_R16G16B16A16_FLOAT)
	{
		ERROR_MESSAGE(NAMEOF(SaveTexture2DToDataset) + L" only supports DXGI_FORMAT_R16G16B16A16_FLOAT!");
		return;
	}

	// Change the description to unsigned 16bit RGBA format and CPU readable
	tmpTextureDesc.Format = DXGI_FORMAT_R16G16B16A16_UINT;
	tmpTextureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	tmpTextureDesc.Usage = D3D11_USAGE_STAGING;
	tmpTextureDesc.BindFlags = 0;

	// Create a temporary texure with that format
	hr = d3d11Device->CreateTexture2D(&tmpTextureDesc, NULL, &tmpTexture);
	ERROR_MESSAGE_ON_FAIL(hr, NAMEOF(d3d11Device->CreateTexture2D) + L" failed!");

	// Copy the content of the actual texture to the temporary texture and convert the format
	d3d11DevCon->CopyResource(tmpTexture, texture);

	// Read the raw texture data
	D3D11_MAPPED_SUBRESOURCE subresource;
	d3d11DevCon->Map(tmpTexture, 0, D3D11_MAP_READ, 0, &subresource);

	// Copy the 16bit RGBA data to the vector buffer
	std::vector<BYTE> buffer;

	////for (UINT i = 0; i < subresource.DepthPitch / 2; i++)
	////{
	////	if ((i + 1) % 4 != 0)
	////	{
	////		USHORT data = ((USHORT*)subresource.pData)[i];
	////		buffer.push_back(255 * ((float)data / USHORT_MAX));
	////	}
	////}

	USHORT* data = (USHORT*)subresource.pData;

	for (UINT y = 0; y < tmpTextureDesc.Height; y++)
	{
		for (UINT x = 0; x < tmpTextureDesc.Width; x++)
		{
			UINT index = 4 * (y * tmpTextureDesc.Width + x);

			float r = 4 * data[index];
			float g = 4 * data[index + 1];
			float b = 4 * data[index + 2];

			buffer.push_back(255 * (r / USHORT_MAX));
			buffer.push_back(255 * (g / USHORT_MAX));
			buffer.push_back(255 * (b / USHORT_MAX));
		}
	}

	// Unmap and release the texture
	d3d11DevCon->Unmap(tmpTexture, 0);
	SAFE_RELEASE(tmpTexture);

	status = H5IMmake_image_24bit(fileID, name.c_str(), (hsize_t)tmpTextureDesc.Width, (hsize_t)tmpTextureDesc.Height, "INTERLACE_PIXEL", buffer.data());

	//// Save in a 3d array
	//hsize_t dimensions[] =
	//{
	//	tmpTextureDesc.Width,
	//	tmpTextureDesc.Height,
	//	4
	//};

	//// Define the dimensions of the dataset
	//hid_t dataspaceID = H5Screate_simple(4, dimensions, NULL);

	//// Create the dataset
	//hid_t datasetID = H5Dcreate(fileID, "/texture", H5T_NATIVE_USHORT, dataspaceID, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

	//// Write the data
	//status = H5Dwrite(datasetID, H5T_NATIVE_USHORT, H5S_ALL, H5S_ALL, H5P_DEFAULT, buffer.data());

	//// Close dataspace and dataset
	//status = H5Dclose(datasetID);
	//status = H5Sclose(dataspaceID);
}
