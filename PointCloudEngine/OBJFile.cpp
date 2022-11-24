#include "PrecompiledHeader.h"
#include "OBJFile.h"

OBJFile::OBJFile(std::wstring filename)
{
    std::wstring directory = filename.substr(0, filename.find_last_of(L"\\/")) + L"/";

    std::wifstream objFile(filename);
    ERROR_MESSAGE_ON_FAIL(objFile.is_open(), L"Cannot open OBJ file \"" + filename + L"\"!");

    // IMPORTANT: The polygonal face elements must all be triangular and have texture as well as normal indices
    // First read vertex positions, texture coordinates, normals and per material triangles into temporary buffers
    std::wstring materialTemplateLibraryFilename = L"";
    std::vector<Vector3> positions;
    std::vector<Vector2> textureCoordinates;
    std::vector<Vector3> normals;
    std::map<std::wstring, std::vector<OBJTriangle>> triangles;
    std::map<std::wstring, std::wstring> materialTextures;
    std::wstring materialName = L"";

    std::wstring line;

    while (std::getline(objFile, line))
    {
        std::cout << std::string(line.begin(), line.end()) << std::endl;

        std::vector<std::wstring> values = Utils::SplitString(line, L" ");

        if (values.size() > 0)
        {
            std::wstring key = values.front();

            if (key == L"mtllib")
            {
                materialTemplateLibraryFilename = values[1];
            }
            else if (key == L"usemtl")
            {
                materialName = values[1];
            }
            else if (key == L"v")
            {
                positions.push_back(Vector3(std::stof(values[1]), std::stof(values[2]), std::stof(values[3])));
            }
            else if (key == L"vt")
            {
                textureCoordinates.push_back(Vector2(std::stof(values[1]), std::stof(values[2])));
            }
            else if (key == L"vn")
            {
                normals.push_back(Vector3(std::stof(values[1]), std::stof(values[2]), std::stof(values[3])));
            }
            else if (key == L"f")
            {
                OBJTriangle objTriangle;

                for (int i = 0; i < 3; i++)
                {
                    std::vector<std::wstring> indices = Utils::SplitString(values[1 + i], L"/");
                    objTriangle.vertexIndices[i].positionIndex = std::stoi(indices[0]);
                    objTriangle.vertexIndices[i].textureCoordinateIndex = std::stoi(indices[1]);
                    objTriangle.vertexIndices[i].normalIndex = std::stoi(indices[2]);
                }

                triangles[materialName].push_back(objTriangle);
            }
        }
    }

    // MTL file is assumed to be in the same directory as the OBJ file
    if (materialTemplateLibraryFilename.length() > 0)
    {
        // Prepend the directory filepath of the OBJ file to the MTL file
        materialTemplateLibraryFilename = directory + materialTemplateLibraryFilename;

        std::wifstream mtlFile(materialTemplateLibraryFilename);
        ERROR_MESSAGE_ON_FAIL(mtlFile.is_open(), L"Cannot open MTL file \"" + materialTemplateLibraryFilename + L"\" that is associated with the OBJ file \"" + filename + L"\"!");

        // For now this just considers diffuse textures
        while (std::getline(mtlFile, line))
        {
            std::cout << std::string(line.begin(), line.end()) << std::endl;

            std::vector<std::wstring> values = Utils::SplitString(line, L" ");

            if (values.size() > 0)
            {
                std::wstring key = values.front();

                if (key == L"newmtl")
                {
                    materialName = values[1];
                }
                else if (key == L"map_Kd")
                {
                    materialTextures[materialName] = directory + values[1];
                }
            }
        }
    }

    // TODO
    // - convert into single vertex buffer and single index buffer
    // - load the textures into a texture array (but they might have different sizes!?)
    std::cout << "TODO" << std::endl;
}

std::vector<Vertex> OBJFile::GetVertices()
{
    return std::vector<Vertex>();
}

std::vector<UINT> OBJFile::GetTriangles()
{
    return std::vector<UINT>();
}

OBJFile::~OBJFile()
{
}
