#include "Octree.hlsl"
#include "OctreeConstantBuffer.hlsl"

StructuredBuffer<OctreeNode> nodesBuffer : register(t0);
StructuredBuffer<OctreeNodeTraversalEntry> vertexBuffer : register(t1);

VS_INPUT VS(uint vertexID : SV_VERTEXID)
{
	OctreeNodeTraversalEntry entry = vertexBuffer[vertexID];
	OctreeNode n = nodesBuffer[entry.index];
    OctreeNodeProperties p = n.properties;

    VS_INPUT input;

	// Convert from the struct in the buffer to the vertex layout
	// The bit order is swapped here to LittleEndian, highest value bits are actually now the most right bits
	input.childrenMask = p.childrenMaskAndWeights & 0xff;
	input.weight0 = (p.childrenMaskAndWeights >> 8) & 0xff;
	input.weight1 = (p.childrenMaskAndWeights >> 16) & 0xff;
	input.weight2 = (p.childrenMaskAndWeights >> 24) & 0xff;
    input.normal0 = p.normal01 & 0xffff;
    input.normal1 = (p.normal01 >> 16) & 0xffff;
	input.normal2 = p.normal23 & 0xffff;
	input.normal3 = (p.normal23 >> 16) & 0xffff;
    input.color0 = p.color01 & 0xffff;
    input.color1 = p.color01 >> 16;
	input.color2 = p.color23 & 0xffff;
	input.color3 = p.color23 >> 16;
    input.size = entry.size;

	// Leaf node: compute a more accurate position from the childrenStartOrLeafPositionFactors
	if (input.childrenMask == 0)
	{
		// Extract the factors from childrenStartOrLeafPositionFactors and compute the more accurate position
		float3 startPosition = entry.position - (0.5f * entry.size * float3(1, 1, 1));

		// Get the factors from the 32bit uint
		float factorX = ((n.childrenStartOrLeafPositionFactors >> 16) & 0xff) / 255.0f;
		float factorY = ((n.childrenStartOrLeafPositionFactors >> 8) & 0xff) / 255.0f;
		float factorZ = (n.childrenStartOrLeafPositionFactors & 0xff) / 255.0f;

		input.position = startPosition + entry.size * float3(factorX, factorY, factorZ);
	}
	else
	{
		input.position = entry.position;
	}

    return input;
}