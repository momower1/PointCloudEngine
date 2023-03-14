#ifndef OBJFILE_H
#define OBJFILE_H

#include "PointCloudEngine.h"

namespace PointCloudEngine
{
	struct OBJMesh
	{
		std::wstring textureFilename;
		std::vector<MeshVertex> triangles;
	};

	struct OBJStructuredBuffers
	{
		std::vector<Vector3> positions;
		std::vector<Vector2> textureCoordinates;
		std::vector<Vector3> normals;
	};

	struct OBJContainer
	{
		OBJStructuredBuffers buffers;
		std::vector<OBJMesh> meshes;
	};

	class OBJFile
	{
	public:
		static bool LoadOBJFile(std::wstring filename, OBJContainer* container);
	};
}
#endif
