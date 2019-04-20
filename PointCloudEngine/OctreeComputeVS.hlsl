#include "OctreeShaderFunctions.hlsl"

StructuredBuffer<OctreeNode> nodesBuffer : register(t0);
StructuredBuffer<OctreeNodeTraversalEntry> vertexBuffer : register(t1);

VS_INPUT VS(uint vertexID : SV_VERTEXID)
{
	OctreeNodeTraversalEntry entry = vertexBuffer[vertexID];
    OctreeNodeProperties p = nodesBuffer[entry.index].properties;

    VS_INPUT input;

	// Convert from the struct in the buffer to the vertex layout
	// The bit order is swapped here to LittleEndian, highest value bits are actually now the most right bits
    input.position = entry.position;
    input.normal0 = uint2(p.normal01 & 0xff, (p.normal01 >> 8) & 0xff);
    input.normal1 = uint2((p.normal01 >> 16) & 0xff, (p.normal01 >> 24) & 0xff);
	input.normal2 = uint2(p.normal23 & 0xff, (p.normal23 >> 8) & 0xff);
	input.normal3 = uint2((p.normal23 >> 16) & 0xff, (p.normal23 >> 24) & 0xff);
	input.normal4 = uint2(p.normal45 & 0xff, (p.normal45 >> 8) & 0xff);
	input.normal5 = uint2((p.normal45 >> 16) & 0xff, (p.normal45 >> 24) & 0xff);
    input.color0 = p.color01 & 0xffff;
    input.color1 = p.color01 >> 16;
	input.color2 = p.color23 & 0xffff;
	input.color3 = p.color23 >> 16;
	input.color4 = p.color45 & 0xffff;
	input.color5 = p.color45 >> 16;
    input.weights = p.weights;
    input.size = entry.size;

    return input;
}