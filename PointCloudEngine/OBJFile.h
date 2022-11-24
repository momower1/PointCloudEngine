#ifndef OBJFILE_H
#define OBJFILE_H

#include "PointCloudEngine.h"

class OBJFile
{
public:
	OBJFile(std::wstring filename);
	std::vector<Vertex> GetVertices();
	std::vector<UINT> GetTriangles();
	~OBJFile();

private:
	struct OBJVertexIndices
	{
		UINT positionIndex;
		UINT textureCoordinateIndex;
		UINT normalIndex;
	};

	struct OBJTriangle
	{
		OBJVertexIndices vertexIndices[3];
	};
};
#endif
