#define PI 3.141592654f
#define EPSILON 1.192092896e-07f
#define UINT_MAX 0xffffffff

struct VS_INPUT
{
    float3 position : POSITION;
    uint2 normal0 : NORMAL0;
    uint2 normal1 : NORMAL1;
    uint2 normal2 : NORMAL2;
    uint2 normal3 : NORMAL3;
    uint color0 : COLOR0;
    uint color1 : COLOR1;
    uint color2 : COLOR2;
    uint color3 : COLOR3;
    uint weight0 : WEIGHT0;
	uint weight1 : WEIGHT1;
	uint weight2 : WEIGHT2;
	uint weight3 : WEIGHT3;
    float size : SIZE;
};

struct GS_OUTPUT
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float3 color : COLOR;
};

struct OctreeNodeTraversalEntry
{
	uint index;
	float3 position;
	float size;
};

struct OctreeNodeProperties
{
	uint normal01;
	uint normal23;
	uint color01;
	uint color23;
	uint weights;
};

struct OctreeNodeVertex
{
	float3 position;
	OctreeNodeProperties properties;
	float size;
};

struct OctreeNode
{
	uint childrenMask;
	uint childrenStart;
    OctreeNodeProperties properties;
};

float3 PolarNormalToFloat3(uint theta, uint phi)
{
    // Theta and phi are in range [0, 255]
    if (theta == 0 && phi == 0)
    {
        return float3(0, 0, 0);
    }

    float t = PI * (theta / 255.0f);
    float p = PI * ((phi / 127.5f) - 1.0f);

    return float3(sin(t) * cos(p), sin(t) * sin(p), cos(t));
}

float3 PolarNormalToFloat3(uint2 polarNormal)
{
    return PolarNormalToFloat3(polarNormal.x, polarNormal.y);
}

float3 Color16ToFloat3(uint color)
{
	// 6 bits red, 6 bits green, 4 bits blue
    float r = ((color >> 10) & 63) / 63.0f;
    float g = ((color >> 4) & 63) / 63.0f;
    float b = (color & 15) / 15.0f;

    return float3(r, g, b);
}