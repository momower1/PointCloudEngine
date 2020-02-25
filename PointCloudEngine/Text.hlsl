Texture2D spritesheet : register(t0);

SamplerState SpritesheetSampler
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Clamp;
    AddressV = Clamp;
};

cbuffer ConstantBufferText : register(b0)
{
    bool worldSpace;
    // 12 bytes auto padding
//------------------------------------------------------------------------------ (16 byte boundary)
    float4 color;
//------------------------------------------------------------------------------ (16 byte boundary)
    float4x4 World;
//------------------------------------------------------------------------------ (64 byte boundary)
    float4x4 View;
//------------------------------------------------------------------------------ (64 byte boundary)
    float4x4 Projection;
//------------------------------------------------------------------------------ (64 byte boundary)
};  // Total: 224 bytes with constant buffer packing rules

struct VS_INPUT
{
    nointerpolation float2 Position : POSITION;
    nointerpolation float3 Offset : OFFSET;
    nointerpolation float4 Rect : RECT;
};

struct VS_OUTPUT
{
    nointerpolation float2 Position : POSITION;
    nointerpolation float3 Offset : OFFSET;
    nointerpolation float4 Rect : RECT;
};

struct GS_OUTPUT
{
    nointerpolation float2 Position : POSITION;
    nointerpolation float3 Offset : OFFSET;
    nointerpolation float4 Rect : RECT;
    float4 ScreenPosition : SV_POSITION;
    float2 UV : TEXCOORD;
};

VS_OUTPUT VS(VS_INPUT input)
{
    VS_OUTPUT output;
    output.Position = input.Position;
    output.Offset = input.Offset;
    output.Rect = input.Rect;

    return output;
}

[maxvertexcount(6)]
void GS(point VS_OUTPUT input[1], inout TriangleStream<GS_OUTPUT> output)
{
    /*

        1,4__5
        |\   |
        | \  |
        |  \ |
        |___\|
        3    2,6

    */

    float4x4 WVP = World;
    float scale = 1.0f / 512;

    if (worldSpace)
    {
        WVP = mul(World, mul(View, Projection));
    }

    float width = (input[0].Rect.z - input[0].Rect.x);
    float height = (input[0].Rect.w - input[0].Rect.y);
    float2 position = input[0].Position.xy + float2(0, -input[0].Offset.y - height);

    // X,Y store the position and ZW the UV coordinate of the billboard vertex
    float4 billboardVertices[] =
    {
        float4(0, height, 0, 0),
        float4(width, 0, 1, 1),
        float4(0, 0, 0, 1),
        float4(0, height, 0, 0),
        float4(width, height, 1, 0),
        float4(width, 0, 1, 1)
    };

    GS_OUTPUT element;
    element.Position = input[0].Position;
    element.Offset = input[0].Offset;
    element.Rect = input[0].Rect;

    for (int i = 0; i < 6; i++)
    {
        float2 scaledPosition = scale * (position + billboardVertices[i].xy);
        element.ScreenPosition = mul(float4(scaledPosition, 0, 1), WVP);
        element.UV = billboardVertices[i].zw;
        output.Append(element);

        if (i == 2)
        {
            output.RestartStrip();
        }
    }
}

float4 PS(GS_OUTPUT input) : SV_TARGET
{
    uint spritesheetWidth;
    uint spritesheetHeight;
    spritesheet.GetDimensions(spritesheetWidth, spritesheetHeight);

    float width = input.Rect.z - input.Rect.x;
    float height = input.Rect.w - input.Rect.y;
    
    float2 uv;
    uv.x = (input.Rect.x + input.UV.x * width) / spritesheetWidth;
    uv.y = (input.Rect.y + input.UV.y * height) / spritesheetHeight;

    float4 s = spritesheet.Sample(SpritesheetSampler, uv);

    if (s.a == 0)
    {
        discard;
    }

    return color;
}