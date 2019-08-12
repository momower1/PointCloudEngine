#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <d3d11.h>
#include <SimpleMath.h>
#include "tinyply.h"

using namespace DirectX::SimpleMath;

struct PlyVertex
{
	// Stores the .ply file vertices
	Vector3 position;
	Vector3 normal;
	unsigned char color[3];
};

struct PointcloudVertex
{
	// Stores the .pointcloud vertices
	Vector3 position;
	char normal[3];
	unsigned char color[3];
};

void PlyToPointcloud(const std::string& plyfile)
{
	std::cout << "Converting \"" << plyfile << "\" to .pointcloud file format...";

	try
	{
		// Load ply file
		std::ifstream ss(plyfile, std::ios::binary);

		tinyply::PlyFile file;
		file.parse_header(ss);

		// Tinyply untyped byte buffers for properties
		std::shared_ptr<tinyply::PlyData> rawPositions, rawNormals, rawColors;

		// Hardcoded properties and elements
		rawPositions = file.request_properties_from_element("vertex", { "x", "y", "z" });
		rawNormals = file.request_properties_from_element("vertex", { "nx", "ny", "nz" });
		rawColors = file.request_properties_from_element("vertex", { "red", "green", "blue" });

		// Read the file
		file.read(ss);

		// Create vertices
		size_t count = rawPositions->count;
		size_t stridePositions = rawPositions->buffer.size_bytes() / count;
		size_t strideNormals = rawNormals->buffer.size_bytes() / count;
		size_t strideColors = rawColors->buffer.size_bytes() / count;

		// When this trows an std::bad_alloc exception, the memory requirement is large -> build with x64
		std::vector<PlyVertex> plyVertices(count);

		// Fill each vertex with its data
		for (int i = 0; i < count; i++)
		{
			std::memcpy(&plyVertices[i].position, rawPositions->buffer.get() + i * stridePositions, stridePositions);
			std::memcpy(&plyVertices[i].normal, rawNormals->buffer.get() + i * strideNormals, strideNormals);
			std::memcpy(&plyVertices[i].color, rawColors->buffer.get() + i * strideColors, strideColors);

			// Make sure that the normals are normalized
			plyVertices[i].normal.Normalize();
		}

		// Convert into .pointcloud vertices (smaller size due to normal quantization)
		std::vector<PointcloudVertex> pointcloudVertices;
		Vector3 minPosition = plyVertices.front().position;
		Vector3 maxPosition = minPosition;

		// Also calculate center and size of the bounding cube that fully encloses the point cloud
		for (UINT i = 0; i < count; i++)
		{
			// Only add vertices with a non zero normal
			if (plyVertices[i].normal.LengthSquared() > 0.5f)
			{
				PointcloudVertex pointcloudVertex;

				pointcloudVertex.position = plyVertices[i].position;
				pointcloudVertex.normal[0] = 127 * plyVertices[i].normal.x;
				pointcloudVertex.normal[1] = 127 * plyVertices[i].normal.y;
				pointcloudVertex.normal[2] = 127 * plyVertices[i].normal.z;
				pointcloudVertex.color[0] = plyVertices[i].color[0];
				pointcloudVertex.color[1] = plyVertices[i].color[1];
				pointcloudVertex.color[2] = plyVertices[i].color[2];

				pointcloudVertices.push_back(pointcloudVertex);

				minPosition = Vector3::Min(minPosition, pointcloudVertex.position);
				maxPosition = Vector3::Max(maxPosition, pointcloudVertex.position);
			}
		}

		Vector3 diagonal = maxPosition - minPosition;
		Vector3 boundingCubePosition = minPosition + 0.5f * diagonal;
		float boundingCubeSize = max(max(diagonal.x, diagonal.y), diagonal.z);

		// Randomly shuffle the vertices in order to be able to easily select the density by looking at the first k entries (used in GroundTruthRenderer)
		std::random_shuffle(pointcloudVertices.begin(), pointcloudVertices.end());

		// Write the .pointcloud file
		std::ofstream pointcloudFile(plyfile.substr(0, plyfile.length() - 3) + "pointcloud", std::ios::out | std::ios::binary);

		// Write the bounding cube position
		pointcloudFile.write((char*)&boundingCubePosition, sizeof(Vector3));

		// Write the bounding cube size
		pointcloudFile.write((char*)&boundingCubeSize, sizeof(float));

		// Write the size of the vector
		UINT vertexCount = pointcloudVertices.size();
		pointcloudFile.write((char*)&vertexCount, sizeof(UINT));

		// Write the vertices data in binary format
		pointcloudFile.write((char*)pointcloudVertices.data(), vertexCount * sizeof(PointcloudVertex));

		pointcloudFile.flush();
		pointcloudFile.close();
	}
	catch (const std::exception& e)
	{
		std::cout << "ERROR" << std::endl;
	}

	std::cout << "DONE" << std::endl;
}

void PointcloudToPly(std::string pointcloudfile)
{
	std::cout << "Converting \"" << pointcloudfile << "\" to .ply file format...";

	try
	{
		// Try to load the point cloud from the file
		std::ifstream file(pointcloudfile, std::ios::in | std::ios::binary);

		// Ignore the bounding cube position and size
		file.ignore(sizeof(Vector3) + sizeof(float));

		// Load the size of the vertices vector
		UINT vertexCount;
		file.read((char*)&vertexCount, sizeof(UINT));

		// Read the binary data directly into the vertices vector
		std::vector<PointcloudVertex> pointcloudVertices(vertexCount);
		file.read((char*)pointcloudVertices.data(), vertexCount * sizeof(PointcloudVertex));

		// Convert to .ply vertices
		std::vector<PlyVertex> plyVertices(vertexCount);

		for (UINT i = 0; i < vertexCount; i++)
		{
			plyVertices[i].position = pointcloudVertices[i].position;
			plyVertices[i].normal.x = pointcloudVertices[i].normal[0] / 127.0f;
			plyVertices[i].normal.y = pointcloudVertices[i].normal[1] / 127.0f;
			plyVertices[i].normal.z = pointcloudVertices[i].normal[2] / 127.0f;
			plyVertices[i].color[0] = pointcloudVertices[i].color[0];
			plyVertices[i].color[1] = pointcloudVertices[i].color[1];
			plyVertices[i].color[2] = pointcloudVertices[i].color[2];
		}

		// Write the .ply file
		std::ofstream plyfile(pointcloudfile.substr(0, pointcloudfile.length() - 10) + "ply", std::ios::out | std::ios::binary);

		// Write the header
		plyfile << "ply" << std::endl;
		plyfile << "format binary_little_endian 1.0" << std::endl;
		plyfile << "element vertex " << vertexCount << std::endl;
		plyfile << "property float x" << std::endl;
		plyfile << "property float y" << std::endl;
		plyfile << "property float z" << std::endl;
		plyfile << "property float nx" << std::endl;
		plyfile << "property float ny" << std::endl;
		plyfile << "property float nz" << std::endl;
		plyfile << "property uchar red" << std::endl;
		plyfile << "property uchar green" << std::endl;
		plyfile << "property uchar blue" << std::endl;
		plyfile << "element face 0" << std::endl;
		plyfile << "end_header" << std::endl;

		// Write the vertices data in binary format
		for (int i = 0; i < vertexCount; i++)
		{
			plyfile.write((char*)&plyVertices[i].position, sizeof(Vector3));
			plyfile.write((char*)&plyVertices[i].normal, sizeof(Vector3));
			plyfile.write((char*)&plyVertices[i].color, 3 * sizeof(char));
		}

		plyfile.flush();
		plyfile.close();
	}
	catch (const std::exception& e)
	{
		std::cout << "ERROR" << std::endl;
	}

	std::cout << "DONE" << std::endl;
}

int main(int argc, char* argv[])
{
	std::cout << "This program converts between .ply and .pointcloud file format!" << std::endl;
	std::cout << "Only ply files with (x,y,z,nx,ny,nz,red,green,blue) format are supported!" << std::endl;
	std::cout << "You can generate this ply format by exporting files with e.g. MeshLab." << std::endl << std::endl;
	
	std::cout << "The .pointcloud file format stores the following binary data:" << std::endl;
	std::cout << "\tVector3 - position of the bounding cube" << std::endl;
	std::cout << "\tfloat - size of the bounding cube" << std::endl;
	std::cout << "\tuint - length of the vertex array" << std::endl;
	std::cout << "\tvector - list of vertices" << std::endl;
	std::cout << "Each vertex consists of:" << std::endl;
	std::cout << "\tVector3 - position" << std::endl;
	std::cout << "\tchar[3] - normalized normal" << std::endl;
	std::cout << "\tuchar[3] - rgb color" << std::endl << std::endl;
	
	std::cout << "Drag and drop .ply files to generate the corresponding .pointcloud files." << std::endl;
	std::cout << "Drag and drop .pointcloud files to generate the original .ply file." << std::endl << std::endl;

	for (int i = 1; i < argc; i++)
	{
		std::string filename(argv[i]);

		// Check if it is a .ply or .pointcloud file
		std::string filetype = filename.substr(filename.find_last_of(".") + 1, filename.length());

		if (filetype.compare("ply") == 0 || filetype.compare("PLY") == 0)
		{
			PlyToPointcloud(filename);
		}
		else if (filetype.compare("pointcloud") == 0 || filetype.compare("POINTCLOUD") == 0)
		{
			PointcloudToPly(filename);
		}
		else
		{
			std::cout << "Error reading file " << filename << std::endl;
			return S_FALSE;
		}
	}

	return S_OK;
}