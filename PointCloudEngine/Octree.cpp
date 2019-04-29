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

		// Stores the indices in the nodes array of the children of a node
		// Will only be used while creating the octree (for simplicity)
		// Finding the correct child index is easier this way
		std::vector<UINT> children;

        // Stores the nodes that should be created for each octree level
        std::queue<OctreeNodeCreationEntry> nodeCreationQueue;

        OctreeNodeCreationEntry rootEntry;
        rootEntry.nodesIndex = UINT_MAX;
        rootEntry.childrenIndex = UINT_MAX;
        rootEntry.vertices = vertices;
        rootEntry.position = rootPosition;
        rootEntry.size = rootSize;
        rootEntry.depth = 0;

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

		// Now the nodes actually store the childrenStartOrLeafPositionFactors index for the children array instead of the nodes array
		for (auto it = nodes.begin(); it != nodes.end(); it++)
		{
			// Overwrite the index with one that is referencing the nodes array (that's fine because the nodes array stores children after each other and in order)
			// Then there is no need to store the children array anymore
			if (it->properties.childrenMask != 0)
			{
				it->childrenStartOrLeafPositionFactors = children[it->childrenStartOrLeafPositionFactors];
			}
		}

        // Save the generated octree in a file
        SaveToOctreeFile();
    }
}

std::vector<OctreeNodeVertex> PointCloudEngine::Octree::GetVertices(const OctreeConstantBuffer &octreeConstantBufferData) const
{
	// If the level is -1 then it is ignored and only the node vertices with the projected size smaller than the splat size are returned
	// Otherwise the camera positiona and splat size is ignored and only the node vertices at the given octree level are returned
    // Use a queue instead of recursion to traverse the octree in the memory layout order (improves cache efficiency)
    std::vector<OctreeNodeVertex> octreeVertices;
	std::queue<OctreeNodeTraversalEntry> nodesQueue;

	// Use this struct to compute the node positions and sizes at runtime
	OctreeNodeTraversalEntry rootEntry;
	rootEntry.index = 0;
	rootEntry.position = rootPosition;
	rootEntry.size = rootSize;
	rootEntry.parentInsideViewFrustum = false;
	rootEntry.depth = 0;

    // Check the root node first
    nodesQueue.push(rootEntry);

    while (!nodesQueue.empty())
    {
		OctreeNodeTraversalEntry entry = nodesQueue.front();
        nodesQueue.pop();

        // Check the node, add the vertex or add its children to the queue
        nodes[entry.index].GetVertices(nodesQueue, octreeVertices, entry, octreeConstantBufferData);
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

        // Read the binary data directly into the nodes vector
        nodes.resize(nodesSize);
        octreeFile.read((char*)nodes.data(), nodesSize * sizeof(OctreeNode));

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

        // Write the nodes data in binary format
        octreeFile.write((char*)nodes.data(), nodesSize * sizeof(OctreeNode));

        octreeFile.flush();
        octreeFile.close();
    }
}
