#include "Octree.h"

PointCloudEngine::Octree::Octree(const std::vector<Vertex> &vertices, const int &depth)
{
    // Reserve vector memory for better performance
    int predictedDepth = 1 + min(depth, log(vertices.size()) / log(8));
    int predictedSize = pow(8, predictedDepth);
    nodes.reserve(predictedSize);

    // Try to load a previously saved octree file first before recreating the whole octree (saves a lot of time)
    std::wstring filename = settings->plyfile.substr(settings->plyfile.find_last_of(L"\\/") + 1, settings->plyfile.length());
    filename = filename.substr(0, filename.length() - 4);
    octreeFilepath = executableDirectory + L"/Octrees/" + filename + L".octree";

    // Try to load the octree from a file
    std::wifstream octreeFile(octreeFilepath);

    // Only save the data when the file doesn't exist already
    if (octreeFile.is_open())
    {
        // Parse the file
        std::wstring line;

        while (std::getline(octreeFile, line))
        {
            std::wstringstream stream(line);

            OctreeNode n;

            stream >> n.nodeVertex.position.x >> n.nodeVertex.position.y >> n.nodeVertex.position.z;
            stream >> n.nodeVertex.size;

            for (int i = 0; i < 6; i++)
            {
                int tmp;
                stream >> tmp;
                n.nodeVertex.normals[i].phi = tmp;
                stream >> tmp;
                n.nodeVertex.normals[i].theta = tmp;
                stream >> n.nodeVertex.colors[i].data;
                stream >> tmp;
                n.nodeVertex.weights[i] = tmp;
            }

            for (int i = 0; i < 6; i++)
            {
                stream >> n.children[i];
            }

            nodes.push_back(n);
        }

        // Stop here after loading the file
        return;
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

    Vector3 diagonal = maxPosition - minPosition;
    Vector3 center = minPosition + 0.5f * (diagonal);
    float size = max(max(diagonal.x, diagonal.y), diagonal.z);

    // Stores the nodes that should be created for each octree level
    std::queue<OctreeNodeCreationEntry> nodeCreationQueue;

    OctreeNodeCreationEntry rootEntry;
    rootEntry.nodeIndex = -1;
    rootEntry.parentIndex = -1;
    rootEntry.parentChildIndex = -1;
    rootEntry.vertices = vertices;
    rootEntry.center = center;
    rootEntry.size = size;
    rootEntry.depth = depth;

    nodeCreationQueue.push(rootEntry);

    while (!nodeCreationQueue.empty())
    {
        // Remove the first entry from the queue
        OctreeNodeCreationEntry first = nodeCreationQueue.front();
        nodeCreationQueue.pop();

        // Assign the index at which this node will be stored
        first.nodeIndex = nodes.size();

        // Create the nodes and fill the queue
        nodes.push_back(OctreeNode(nodeCreationQueue, nodes, first));
    }
}

PointCloudEngine::Octree::~Octree()
{
    // Try to open a previously saved file
    std::wifstream file(octreeFilepath);

    // Only save the data when the file doesn't exist already
    if (!file.is_open())
    {
        // Save the octree in a file inside a new folder
        CreateDirectory((executableDirectory + L"/Octrees").c_str(), NULL);
        std::wofstream octreeFile(octreeFilepath);

        for (auto it = nodes.begin(); it != nodes.end(); it++)
        {
            OctreeNode n = *it;

            // TODO: Save all properties in byte representation
            octreeFile << n.nodeVertex.position.x << L" " << n.nodeVertex.position.y << L" " << n.nodeVertex.position.z << L" ";
            octreeFile << n.nodeVertex.size << L" ";

            for (int i = 0; i < 6; i++)
            {
                octreeFile << n.nodeVertex.normals[i].phi << L" " << n.nodeVertex.normals[i].theta << L" ";
                octreeFile << n.nodeVertex.colors[i].data << L" ";
                octreeFile << n.nodeVertex.weights[i] << L" ";
            }

            for (int i = 0; i < 6; i++)
            {
                octreeFile << n.children[i] << L" ";
            }

            octreeFile << std::endl;
        }

        octreeFile.flush();
        octreeFile.close();
    }

    nodes.clear();
}

std::vector<OctreeNodeVertex> PointCloudEngine::Octree::GetVertices(const Vector3 &localCameraPosition, const float &splatSize) const
{
    // Use a queue instead of recursion to traverse the octree in the memory layout order (improves cache efficiency)
    std::vector<OctreeNodeVertex> octreeVertices;
    std::queue<int> nodesQueue;

    // Check the root node first
    nodesQueue.push(0);

    while (!nodesQueue.empty())
    {
        int nodeIndex = nodesQueue.front();
        nodesQueue.pop();

        // Check the node, add the vertex or add its children to the queue
        nodes[nodeIndex].GetVertices(nodesQueue, octreeVertices, localCameraPosition, splatSize);
    }

    return octreeVertices;
}

std::vector<OctreeNodeVertex> PointCloudEngine::Octree::GetVerticesAtLevel(const int &level) const
{
    // Use a queue instead of recursion to traverse the octree in the memory layout order (improves cache efficiency)
    // The queue stores the indices of the nodes that need to be checked and the level of that node
    std::vector<OctreeNodeVertex> octreeVertices;
    std::queue<std::pair<int, int>> nodesQueue;

    // Check the root node first
    nodesQueue.push(std::pair<int, int>(0, level));

    while (!nodesQueue.empty())
    {
        std::pair<int, int> nodePair = nodesQueue.front();
        nodesQueue.pop();

        // Check the node, add the vertex or add its children to the queue
        nodes[nodePair.first].GetVerticesAtLevel(nodesQueue, octreeVertices, nodePair.second);
    }

    return octreeVertices;
}

void PointCloudEngine::Octree::GetRootPositionAndSize(Vector3 &outRootPosition, float &outSize) const
{
    outRootPosition = nodes[0].nodeVertex.position;
    outSize = nodes[0].nodeVertex.size;
}