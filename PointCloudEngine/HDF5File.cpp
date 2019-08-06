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

	// Change the output description to unsigned 8bit RGBA format and render target
	D3D11_TEXTURE2D_DESC outputTextureDesc = inputTextureDesc;
	outputTextureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
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

	// Perform texture conversion (also converts back to linear color space from gamma space)
	d3d11DevCon->Draw(1, 0);

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

	// Copy the 16bit RGBA data to the vector buffer
	std::vector<BYTE> buffer;

	// Ignore alpha component
	for (UINT i = 0; i < subresource.DepthPitch; i++)
	{
		if ((i + 1) % 4 != 0)
		{
			buffer.push_back(((BYTE*)subresource.pData)[i]);
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

	// Create an image in the hdf5 file
	status = H5IMmake_image_24bit(fileID, name.c_str(), (hsize_t)inputTextureDesc.Width, (hsize_t)inputTextureDesc.Height, "INTERLACE_PIXEL", buffer.data());
}
