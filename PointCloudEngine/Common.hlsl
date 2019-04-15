#define PI 3.141592654f
#define EPSILON 1.192092896e-07f
#define UINT_MAX 0xffffffff

struct OctreeNodeVertex
{
    float3 position;
    uint normals[3];
    uint colors[3];
    uint weights;
    float size;
};

struct OctreeNode
{
    uint children[8];
    OctreeNodeVertex nodeVertex;
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

float3 PolarNormalToFloat3(uint polarNormal)
{
    return PolarNormalToFloat3(polarNormal >> 8, polarNormal & 0xff);
}

float3 Color16ToFloat3(uint color)
{
    float r = ((color >> 10) & 63) / 63.0f;
    float g = ((color >> 4) & 63) / 63.0f;
    float b = (color & 15) / 15.0f;

    return float3(r, g, b);
}