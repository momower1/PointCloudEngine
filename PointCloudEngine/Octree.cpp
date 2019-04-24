#include "Octree.h"

PointCloudEngine::Octree::Octree(const std::wstring &plyfile)
{
    if (!LoadFromOctreeFile())
    {
        // Try to load .ply file here
        std::vector<Vertex> vertices;

        if (!LoadPlyFile(vertices, plyfile))
        {
            throw std::exception("Could not load .ply file!");
        }

        // Calculate center and size of the root node
        Vector3 minPosition = vertices.front().position;
        Vector3 maxPosition = minPosition;

        for (auto it = vertices.begin(); it != vertices.end(); it++)
        {
            Vertex v = *it;

            minPosition = Vector3::Min(minPosition, v.position);
            maxPosition = Vector3::Max(maxPosition, v.position);
        }

		// Compute the root node position and size, will be used to compute all the other node positions and sizes at runtime
        Vector3 diagonal = maxPosition - minPosition;
        rootPosition = minPosition + 0.5f * (diagonal);
        rootSize = max(max(diagonal.x, diagonal.y), diagonal.z);

        // Reserve vector memory for better performance
		// This is the size if the vertices would perfectly split by 8 into the octree (always smaller than the real size)
		UINT predictedSize = 0;
        UINT predictedDepth = min(settings->maxOctreeDepth , log(vertices.size()) / log(8));

		for (UINT i = 0; i <= predictedDepth; i++)
		{
			predictedSize += pow(8, i);
		}

        nodes.reserve(predictedSize);
		children.reserve(predictedSize);

        // Stores the nodes that should be created for each octree level
        std::queue<OctreeNodeCreationEntry> nodeCreationQueue;

        OctreeNodeCreationEntry rootEntry;
        rootEntry.nodesIndex = UINT_MAX;
        rootEntry.childrenIndex = UINT_MAX;
        rootEntry.vertices = vertices;
        rootEntry.position = rootPosition;
        rootEntry.size = rootSize;
        rootEntry.depth = settings->maxOctreeDepth;

        nodeCreationQueue.push(rootEntry);

        while (!nodeCreationQueue.empty())
        {
            // Remove the first entry from the queue
            OctreeNodeCreationEntry first = nodeCreationQueue.front();
            nodeCreationQueue.pop();

            // Assign the index at which this node will be stored
            first.nodesIndex = nodes.size();

            // Create the nodes and fill the queue
            nodes.push_back(OctreeNode(nodeCreationQueue, nodes, children, first));
        }

        // Save the generated octree in a file
        SaveToOctreeFile();
    }
}

std::vector<OctreeNodeVertex> PointCloudEngine::Octree::GetVertices(const Vector3 &localCameraPosition, const float &splatSize) const
{
    // Use a queue instead of recursion to traverse the octree in the memory layout order (improves cache efficiency)
    std::vector<OctreeNodeVertex> octreeVertices;
	std::queue<OctreeNodeTraversalEntry> nodesQueue;

	// Use this struct to compute the node positions and sizes at runtime
	OctreeNodeTraversalEntry rootEntry;
	rootEntry.index = 0;
	rootEntry.position = rootPosition;
	rootEntry.size = rootSize;

    // Check the root node first
    nodesQueue.push(rootEntry);

    while (!nodesQueue.empty())
    {
		OctreeNodeTraversalEntry entry = nodesQueue.front();
        nodesQueue.pop();

        // Check the node, add the vertex or add its children to the queue
        nodes[entry.index].GetVertices(nodesQueue, octreeVertices, children, entry, localCameraPosition, splatSize);
    }

    return octreeVertices;
}

std::vector<OctreeNodeVertex> PointCloudEngine::Octree::GetVerticesAtLevel(const int &level) const
{
    // Use a queue instead of recursion to traverse the octree in the memory layout order (improves cache efficiency)
    // The queue stores the indices of the nodes that need to be checked and the level of that node
    std::vector<OctreeNodeVertex> octreeVertices;
    std::queue<std::pair<OctreeNodeTraversalEntry, int>> nodesQueue;

	// Use this struct to compute the node positions and sizes at runtime
	OctreeNodeTraversalEntry rootEntry;
	rootEntry.index = 0;
	rootEntry.position = rootPosition;
	rootEntry.size = rootSize;

    // Check the root node first
    nodesQueue.push(std::pair<OctreeNodeTraversalEntry, int>(rootEntry, level));

    while (!nodesQueue.empty())
    {
        std::pair<OctreeNodeTraversalEntry, int> nodePair = nodesQueue.front();
        nodesQueue.pop();

        // Check the node, add the vertex or add its children to the queue
        nodes[nodePair.first.index].GetVerticesAtLevel(nodesQueue, octreeVertices, children, nodePair.first, nodePair.second);
    }

    return octreeVertices;
}

bool PointCloudEngine::Octree::LoadFromOctreeFile()
{
    // Try to load a previously saved octree file first before recreating the whole octree (saves a lot of time)
    std::wstring filename = settings->plyfile.substr(settings->plyfile.find_last_of(L"\\/") + 1, settings->plyfile.length());
    filename = filename.substr(0, filename.length() - 4);
    octreeFilepath = executableDirectory + L"/Octrees/" + filename + L".octree";

    // Try to load the octree from a file
    std::ifstream octreeFile(octreeFilepath, std::ios::in | std::ios::binary);

    // Only save the data when the file doesn't exist already
    if (octreeFile.is_open())
    {
		// Read the root position as the first entry
		octreeFile.read((char*)&rootPosition, sizeof(Vector3));

		// Then the root size
		octreeFile.read((char*)&rootSize, sizeof(float));

        // Load the size of the nodes vector
        UINT nodesSize;
        octreeFile.read((char*)&nodesSize, sizeof(UINT));

		// Load the size of the children vector
		UINT childrenSize;
		octreeFile.read((char*)&childrenSize, sizeof(UINT));

        // Read the binary data directly into the nodes vector
        nodes.resize(nodesSize);
        octreeFile.read((char*)nodes.data(), nodesSize * sizeof(OctreeNode));

		// Read the binary data directly into the children vector
		children.resize(childrenSize);
		octreeFile.read((char*)children.data(), childrenSize * sizeof(UINT));

        // Stop here after loading the file
        return true;
    }

    return false;
}

void PointCloudEngine::Octree::SaveToOctreeFile()
{
    // Try to open a previously saved file
    std::wifstream file(octreeFilepath);

    // Only save the data when the file doesn't exist already
    if (!file.is_open())
    {
        // Save the octree in a file inside a new folder
        CreateDirectory((executableDirectory + L"/Octrees").c_str(), NULL);
        std::ofstream octreeFile(octreeFilepath, std::ios::out | std::ios::binary);

		// Write the root position
		octreeFile.write((char*)&rootPosition, sizeof(Vector3));

		// Then the root size
		octreeFile.write((char*)&rootSize, sizeof(float));

        // Write the size of the nodes vector
        UINT nodesSize = nodes.size();
        octreeFile.write((char*)&nodesSize, sizeof(UINT));

		// Write the size of the children vector
		UINT childrenSize = children.size();
		octreeFile.write((char*)&childrenSize, sizeof(UINT));

        // Write the nodes data in binary format
        octreeFile.write((char*)nodes.data(), nodesSize * sizeof(OctreeNode));

		// Write the children data in binary format
		octreeFile.write((char*)children.data(), childrenSize * sizeof(UINT));

        octreeFile.flush();
        octreeFile.close();
    }
}
