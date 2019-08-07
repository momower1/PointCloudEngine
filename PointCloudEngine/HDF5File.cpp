#include "HDF5File.h"

HDF5File::HDF5File(std::wstring filename)
{
	file = new H5::H5File(std::string(filename.begin(), filename.end()).c_str(), H5F_ACC_TRUNC);
}

HDF5File::HDF5File(std::string filename)
{
	file = new H5::H5File(filename.c_str(), H5F_ACC_TRUNC);
}

HDF5File::~HDF5File()
{
	delete file;
}

void HDF5File::AddColorTextureDataset(std::wstring name, ID3D11Texture2D* texture, bool gammaSpace)
{
	AddColorTextureDataset(std::string(name.begin(), name.end()), texture, gammaSpace);
}

void HDF5File::AddColorTextureDataset(std::string name, ID3D11Texture2D* texture, bool gammaSpace)
{
	// 1. Convert the input RGBA texture into a 32bit RGBA texture
	// 2. Make it readable by the CPU and convert only the RGB content to a 8bit buffer (skip alpha)
	// 3. Use the HDF5 high level API to create an image dataset from this buffer
	ID3D11Texture2D* inputTexture = NULL;
	ID3D11Texture2D* outputTexture = NULL;
	ID3D11Texture2D* readableTexture = NULL;
	ID3D11RenderTargetView* outputTextureRTV = NULL;
	ID3D11ShaderResourceView* inputTextureSRV = NULL;

	// Get the texture description
	D3D11_TEXTURE2D_DESC inputTextureDesc;
	texture->GetDesc(&inputTextureDesc);

	// Change the bind flag to make it possible to access the texture in a shader
	inputTextureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	// Create the input texture
	hr = d3d11Device->CreateTexture2D(&inputTextureDesc, NULL, &inputTexture);
	ERROR_MESSAGE_ON_FAIL(hr, NAMEOF(d3d11Device->CreateTexture2D) + L" failed!");

	// Create a shader resource view for the input texture
	D3D11_SHADER_RESOURCE_VIEW_DESC inputTextureSRVDesc;
	ZeroMemory(&inputTextureSRVDesc, sizeof(inputTextureSRVDesc));
	inputTextureSRVDesc.Format = inputTextureDesc.Format;
	inputTextureSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	inputTextureSRVDesc.Texture2D.MipLevels = 1;

	hr = d3d11Device->CreateShaderResourceView(inputTexture, &inputTextureSRVDesc, &inputTextureSRV);
	ERROR_MESSAGE_ON_FAIL(hr, NAMEOF(d3d11Device->CreateShaderResourceView) + L" failed for the " + NAMEOF(inputTexture));

	// Copy the content to the input texture
	d3d11DevCon->CopyResource(inputTexture, texture);

	// Change the output description to unsigned 32bit RGBA format and render target
	D3D11_TEXTURE2D_DESC outputTextureDesc = inputTextureDesc;
	outputTextureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	outputTextureDesc.BindFlags = D3D11_BIND_RENDER_TARGET;
	outputTextureDesc.Usage = D3D11_USAGE_DEFAULT;
	outputTextureDesc.CPUAccessFlags = 0;

	// Create a temporary texure with that format
	hr = d3d11Device->CreateTexture2D(&outputTextureDesc, NULL, &outputTexture);
	ERROR_MESSAGE_ON_FAIL(hr, NAMEOF(d3d11Device->CreateTexture2D) + L" failed!");

	// Create render target view for this texture
	hr = d3d11Device->CreateRenderTargetView(outputTexture, NULL, &outputTextureRTV);
	ERROR_MESSAGE_ON_FAIL(hr, NAMEOF(d3d11Device->CreateRenderTargetView) + L" failed!");

	// Set the shader and resources that will be used for the texture conversion
	d3d11DevCon->VSSetShader(textureConversionShader->vertexShader, NULL, 0);
	d3d11DevCon->GSSetShader(textureConversionShader->geometryShader, NULL, 0);
	d3d11DevCon->PSSetShader(textureConversionShader->pixelShader, NULL, 0);
	d3d11DevCon->PSSetShaderResources(0, 1, &inputTextureSRV);
	d3d11DevCon->OMSetRenderTargets(1, &outputTextureRTV, NULL);

	// Perform texture conversion
	d3d11DevCon->Draw(1, 0);

	// Reset shaders, resources and render target
	d3d11DevCon->VSSetShader(NULL, NULL, 0);
	d3d11DevCon->GSSetShader(NULL, NULL, 0);
	d3d11DevCon->PSSetShader(NULL, NULL, 0);
	d3d11DevCon->PSSetShaderResources(0, 1, nullSRV);
	d3d11DevCon->OMSetRenderTargets(1, &renderTargetView, depthStencilView);

	// Change the output description to unsigned 8bit RGBA format, render target and CPU readable
	D3D11_TEXTURE2D_DESC readableTextureDesc = outputTextureDesc;
	readableTextureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	readableTextureDesc.Usage = D3D11_USAGE_STAGING;
	readableTextureDesc.BindFlags = 0;

	// Create a temporary CPU readable texure with that format
	hr = d3d11Device->CreateTexture2D(&readableTextureDesc, NULL, &readableTexture);
	ERROR_MESSAGE_ON_FAIL(hr, NAMEOF(d3d11Device->CreateTexture2D) + L" failed!");

	// Copy the data from the output texture to the readable texture
	d3d11DevCon->CopyResource(readableTexture, outputTexture);

	// Read the raw texture data
	D3D11_MAPPED_SUBRESOURCE subresource;
	d3d11DevCon->Map(readableTexture, 0, D3D11_MAP_READ, 0, &subresource);

	// Create a vector for the 8bit RGB data
	std::vector<BYTE> buffer;

	// Convert from 32bit float to 8bit and ignore alpha component
	for (UINT i = 0; i < subresource.DepthPitch / 4; i++)
	{
		if ((i + 1) % 4 != 0)
		{
			float f = ((float*)subresource.pData)[i];

			// Convert from gamma space to linear space if needed
			if (gammaSpace)
			{
				f = std::pow(f, 1.0f / 2.2f);
			}

			buffer.push_back(f * 255);
		}
	}

	// Unmap the texture
	d3d11DevCon->Unmap(readableTexture, 0);

	// Release the resources
	SAFE_RELEASE(inputTexture);
	SAFE_RELEASE(outputTexture);
	SAFE_RELEASE(readableTexture);
	SAFE_RELEASE(outputTextureRTV);
	SAFE_RELEASE(inputTextureSRV);

	// TODO: Compress like below using a custom R8G8B8 3D array
	// https://support.hdfgroup.org/HDF5/doc/ADGuide/ImageSpec.html

	// Create an image in the hdf5 file (24bit = R8G8B8)
	//H5IMmake_image_24bit(file->getId(), name.c_str(), (hsize_t)inputTextureDesc.Width, (hsize_t)inputTextureDesc.Height, "INTERLACE_PIXEL", buffer.data());
}

void HDF5File::AddDepthTextureDataset(std::wstring name, ID3D11Texture2D* texture)
{
	AddDepthTextureDataset(std::string(name.begin(), name.end()), texture);
}

void HDF5File::AddDepthTextureDataset(std::string name, ID3D11Texture2D* texture)
{
	ID3D11Texture2D* readableTexture = NULL;

	// Get the texture description
	D3D11_TEXTURE2D_DESC textureDesc;
	texture->GetDesc(&textureDesc);

	if (textureDesc.Format != DXGI_FORMAT_R32_TYPELESS)
	{
		ERROR_MESSAGE(NAMEOF(AddDepthTextureDataset) + L" only supports textures with DXGI_FORMAT_R32_TYPELESS format!");
		return;
	}

	// Change the description to make the texture CPU readable
	textureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	textureDesc.Usage = D3D11_USAGE_STAGING;
	textureDesc.BindFlags = 0;

	// Create a readable temporary texture
	hr = d3d11Device->CreateTexture2D(&textureDesc, NULL, &readableTexture);
	ERROR_MESSAGE_ON_FAIL(hr, NAMEOF(d3d11Device->CreateTexture2D) + L" failed!");

	// Copy the content of the original texture
	d3d11DevCon->CopyResource(readableTexture, texture);

	// Read the raw texture data
	D3D11_MAPPED_SUBRESOURCE subresource;
	d3d11DevCon->Map(readableTexture, 0, D3D11_MAP_READ, 0, &subresource);

	// Save in a HDF5 2D array
	hsize_t dimensions[] =
	{
		textureDesc.Height,
		textureDesc.Width
	};

	// Define the dimensions of the dataset
	H5::DataSpace dataspace(2, dimensions);

	// Chunk the data in order to use compression
	hsize_t chunkDimensions[] =
	{
		20,
		20
	};

	// Create a chunk property to set up the ZLIB deflate compression
	H5::DSetCreatPropList proplist;
	proplist.setChunk(2, chunkDimensions);
	proplist.setDeflate(6);

	// Create the dataset
	H5::DataSet dataset = file->createDataSet(name.c_str(), H5::PredType::NATIVE_FLOAT, dataspace, proplist);

	// Create attributes so that this data is interpreted as an image
	H5::DataSpace attributeDataspace(H5S_SCALAR);
	H5::StrType classType(H5::PredType::C_S1, 5);
	H5::Attribute classAttribute = dataset.createAttribute("CLASS", classType, attributeDataspace);
	classAttribute.write(classType, std::string("IMAGE"));

	H5::StrType versionType(H5::PredType::C_S1, 3);
	H5::Attribute versionAttribute = dataset.createAttribute("IMAGE_VERSION", versionType, attributeDataspace);
	versionAttribute.write(versionType, std::string("1.2"));

	// Write the data
	dataset.write(subresource.pData, H5::PredType::NATIVE_FLOAT);
	
	// Unmap the texture
	d3d11DevCon->Unmap(readableTexture, 0);

	SAFE_RELEASE(readableTexture);
}
