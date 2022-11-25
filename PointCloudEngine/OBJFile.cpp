#include "PrecompiledHeader.h"
#include "OBJFile.h"

OBJContainer OBJFile::LoadOBJFile(std::wstring filename)
{
    std::wstring directory = filename.substr(0, filename.find_last_of(L"\\/")) + L"/";

    std::wifstream objFile(filename);
    ERROR_MESSAGE_ON_FAIL(objFile.is_open(), L"Cannot open OBJ file \"" + filename + L"\"!");

    // IMPORTANT: The polygonal face elements must all be triangular and have texture as well as normal indices
    // First read vertex positions, texture coordinates, normals and per material triangles into the buffers
    OBJContainer container;

    std::wstring materialTemplateLibraryFilename = L"";
    std::map<std::wstring, std::vector<MeshVertex>> submeshes;
    std::map<std::wstring, std::wstring> materialTextures;
    std::wstring materialName = L"";

    std::wstring line;

    while (std::getline(objFile, line))
    {
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
                container.buffers.positions.push_back(Vector3(std::stof(values[1]), std::stof(values[2]), std::stof(values[3])));
            }
            else if (key == L"vt")
            {
                // Need to invert texture coordinates for DirectX texture space
                container.buffers.textureCoordinates.push_back(Vector2(1.0f - std::stof(values[1]), 1.0f - std::stof(values[2])));
            }
            else if (key == L"vn")
            {
                container.buffers.normals.push_back(Vector3(std::stof(values[1]), std::stof(values[2]), std::stof(values[3])));
            }
            else if (key == L"f")
            {
                for (int i = 0; i < 3; i++)
                {
                    std::vector<std::wstring> indices = Utils::SplitString(values[1 + i], L"/");

                    // Parse the vertex indices (need to substract 1 since OBJ indices start at 1 instead of 0)
                    MeshVertex vertex;
                    vertex.positionIndex = std::stoi(indices[0]) - 1;
                    vertex.textureCoordinateIndex = std::stoi(indices[1]) - 1;
                    vertex.normalIndex = std::stoi(indices[2]) - 1;

                    submeshes[materialName].push_back(vertex);
                }
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

    for (auto itSubmesh = submeshes.begin(); itSubmesh != submeshes.end(); itSubmesh++)
    {
        OBJMesh mesh;
        mesh.textureFilename = materialTextures[itSubmesh->first];
        mesh.triangles = itSubmesh->second;

        container.meshes.push_back(mesh);
    }

    return container;
}
