cbuffer ConstantBufferMatrices : register(b0)
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
    float3 normal : NORMAL;
    uint4 color : COLOR;
};

struct VS_OUTPUT
{
    float3 position : POSITION;
    float3 normal : NORMAL;
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
    output.normal = input.normal;
    output.color = input.color / 255.0f;

    return output;
}

[maxvertexcount(6)]
void GS(point VS_OUTPUT input[1], inout PointStream<GS_OUTPUT> output) // TODO: Use TriangleStream
{
    ///*

    //    1,4__5
    //    |\   |
    //    | \  |
    //    |  \ |
    //    |___\|
    //    3    2,6

    //*/

    //float4x4 WVP = World;
    //float scale = 1.0f / 512;

    //if (worldSpace)
    //{
    //    WVP = mul(World, mul(View, Projection));
    //}

    //float width = (input[0].Rect.z - input[0].Rect.x);
    //float height = (input[0].Rect.w - input[0].Rect.y);
    //float2 position = input[0].Position.xy + float2(0, -input[0].Offset.y - height);

    //// X,Y store the position and ZW the UV coordinate of the billboard vertex
    //float4 billboardVertices[] =
    //{
    //    float4(0, height, 0, 0),
    //    float4(width, 0, 1, 1),
    //    float4(0, 0, 0, 1),
    //    float4(0, height, 0, 0),
    //    float4(width, height, 1, 0),
    //    float4(width, 0, 1, 1)
    //};

    //GS_OUTPUT element;
    //element.Position = input[0].Position;
    //element.Offset = input[0].Offset;
    //element.Rect = input[0].Rect;

    //for (int i = 0; i < 6; i++)
    //{
    //    float2 scaledPosition = scale * (position + billboardVertices[i].xy);
    //    element.ScreenPosition = mul(float4(scaledPosition, 0, 1), WVP);
    //    element.UV = billboardVertices[i].zw;
    //    output.Append(element);

    //    if (i == 2)
    //    {
    //        output.RestartStrip();
    //    }
    //}

    float4x4 WVP = mul(World, mul(View, Projection));

    GS_OUTPUT element;
    element.position = mul(float4(input[0].position, 1), WVP);
    element.color = input[0].color;

    output.Append(element);
}

float4 PS(GS_OUTPUT input) : SV_TARGET
{
    return input.color;
}