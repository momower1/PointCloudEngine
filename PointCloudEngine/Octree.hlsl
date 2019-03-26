cbuffer OctreeRendererConstantBuffer : register(b0)
{
    float4x4 World;
//------------------------------------------------------------------------------ (64 byte boundary)
    float4x4 View;
//------------------------------------------------------------------------------ (64 byte boundary)
    float4x4 Projection;
//------------------------------------------------------------------------------ (64 byte boundary)
    float4x4 WorldInverseTranspose;
//------------------------------------------------------------------------------ (64 byte boundary)
};  // Total: 256 bytes with constant buffer packing rules

struct VS_INPUT
{
    float3 position : POSITION;
    float3 normal0 : NORMAL0;
    float3 normal1 : NORMAL1;
    float3 normal2 : NORMAL2;
    float3 normal3 : NORMAL3;
    float3 normal4 : NORMAL4;
    float3 normal5 : NORMAL5;
    float4 color0 : COLOR0;
    float4 color1 : COLOR1;
    float4 color2 : COLOR2;
    float4 color3 : COLOR3;
    float4 color4 : COLOR4;
    float4 color5 : COLOR5;
    float size : SIZE;
};

struct VS_OUTPUT
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float size : SIZE;
    float4 color : COLOR;
};

struct GS_OUTPUT
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

VS_OUTPUT VS(VS_INPUT input)
{
    VS_OUTPUT output;
    output.position = input.position;
    output.normal = normalize(mul(input.normal0, WorldInverseTranspose));
    output.size = input.size;
    output.color = input.color0;

    return output;
}

[maxvertexcount(16)]
void GS(point VS_OUTPUT input[1], inout LineStream<GS_OUTPUT> output)
{
    float3 x = float3(input[0].size, 0, 0);
    float3 y = float3(0, input[0].size, 0);
    float3 z = float3(0, 0, input[0].size);

    float4x4 WVP = mul(World, mul(View, Projection));

    GS_OUTPUT element;
    element.color = input[0].color;

    float3 cube[] =
    {
        input[0].position - x - y - z,
        input[0].position + x - y - z,
        input[0].position + x + y - z,
        input[0].position - x + y - z,
        input[0].position - x + y + z,
        input[0].position - x - y + z,
        input[0].position + x - y + z,
        input[0].position + x + y + z
    };

    element.position = mul(float4(cube[3], 1), WVP);
    output.Append(element);
    element.position = mul(float4(cube[0], 1), WVP);
    output.Append(element);
    element.position = mul(float4(cube[1], 1), WVP);
    output.Append(element);
    element.position = mul(float4(cube[2], 1), WVP);
    output.Append(element);
    element.position = mul(float4(cube[3], 1), WVP);
    output.Append(element);
    element.position = mul(float4(cube[4], 1), WVP);
    output.Append(element);
    element.position = mul(float4(cube[5], 1), WVP);
    output.Append(element);
    element.position = mul(float4(cube[6], 1), WVP);
    output.Append(element);
    element.position = mul(float4(cube[7], 1), WVP);
    output.Append(element);
    element.position = mul(float4(cube[4], 1), WVP);
    output.Append(element);

    output.RestartStrip();

    element.position = mul(float4(cube[2], 1), WVP);
    output.Append(element);
    element.position = mul(float4(cube[7], 1), WVP);
    output.Append(element);

    output.RestartStrip();

    element.position = mul(float4(cube[1], 1), WVP);
    output.Append(element);
    element.position = mul(float4(cube[6], 1), WVP);
    output.Append(element);

    output.RestartStrip();

    element.position = mul(float4(cube[0], 1), WVP);
    output.Append(element);
    element.position = mul(float4(cube[5], 1), WVP);
    output.Append(element);

}

float4 PS(GS_OUTPUT input) : SV_TARGET
{
    return input.color;
}