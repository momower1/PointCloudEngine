#include "Octree.hlsl"

cbuffer ComputeShaderConstantBuffer : register(b0)
{
    float3 localCameraPosition;
    float fovAngleY;
//------------------------------------------------------------------------------ (16 byte boundary)
    float splatSize;
	int level;
    uint inputCount;
//  4 bytes auto padding
};  // Total: 32 bytes with constant buffer packing rules

StructuredBuffer<OctreeNode> nodesBuffer : register(t0);
ConsumeStructuredBuffer<OctreeNodeTraversalEntry> inputConsumeBuffer : register(u0);
AppendStructuredBuffer<OctreeNodeTraversalEntry> outputAppendBuffer : register(u1);
AppendStructuredBuffer<OctreeNodeTraversalEntry> vertexAppendBuffer : register(u2);

[numthreads(1024, 1, 1)]
void CS (uint3 id : SV_DispatchThreadID)
{
    // Make sure that this thread is working on valid data
    if (id.x < inputCount)
    {
        // Get some entry that this thread has to check
		OctreeNodeTraversalEntry entry = inputConsumeBuffer.Consume();
        OctreeNode node = nodesBuffer[entry.index];

		// Get the childrenMask
		uint childrenMask = node.properties.childrenMaskAndWeights & 0xff;

		bool traverseChildren = true;

		// Check if only to return the vertices at the given level
		if (level >= 0)
		{
			if (entry.depth == level)
			{
				// Draw this vertex and don't traverse further
				traverseChildren = false;
				vertexAppendBuffer.Append(entry);
			}
		}
		else
		{
			// Calculate required splat size
			float distanceToCamera = distance(localCameraPosition, entry.position);
			float requiredSplatSize = splatSize * (2.0f * tan(fovAngleY / 2.0f)) * distanceToCamera;

			// Check against required splat size and draw this vertex if it is smaller
			if (entry.size < requiredSplatSize || childrenMask == 0)
			{
				traverseChildren = false;
				vertexAppendBuffer.Append(entry);
			}
		}

        if (traverseChildren)
        {
			float childExtend = 0.25f * entry.size;

			float3 childPositions[8] =
			{
				entry.position + float3(childExtend, childExtend, childExtend),
				entry.position + float3(childExtend, childExtend, -childExtend),
				entry.position + float3(childExtend, -childExtend, childExtend),
				entry.position + float3(childExtend, -childExtend, -childExtend),
				entry.position + float3(-childExtend, childExtend, childExtend),
				entry.position + float3(-childExtend, childExtend, -childExtend),
				entry.position + float3(-childExtend, -childExtend, childExtend),
				entry.position + float3(-childExtend, -childExtend, -childExtend)
			};

			uint count = 0;

            // Check all the children in the next compute shader iteration
            for (int i = 0; i < 8; i++)
            {
				if (childrenMask & (1 << i))
				{
					OctreeNodeTraversalEntry childEntry;
					childEntry.index = node.childrenStart + count;
					childEntry.position = childPositions[i];
					childEntry.size = entry.size * 0.5f;
					childEntry.depth = entry.depth + 1;

					outputAppendBuffer.Append(childEntry);

					count++;
				}
            }
        }
    }
}