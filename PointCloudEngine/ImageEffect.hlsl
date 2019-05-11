RWTexture2D<float4> backBufferTexture : register(u1);

struct VS_INPUT
{
	uint vertexID : SV_VERTEXID;
};

struct GS_INOUTPUT
{
	float4 position : SV_POSITION;
};

GS_INOUTPUT VS(VS_INPUT input)
{
    return (GS_INOUTPUT)0;
}

[maxvertexcount(6)]
void GS(point GS_INOUTPUT input[1], inout TriangleStream<GS_INOUTPUT> output)
{
    /*

        1,4__5
        |\   |
        | \  |
        |  \ |
        |___\|
        3    2,6

    */

	// Append all the vertices in the correct order to create the billboard
	GS_INOUTPUT element;
    element.position = float4(-1, 1, 0, 1);
    output.Append(element);

    element.position = float4(1, -1, 0, 1);
    output.Append(element);

    element.position = float4(-1, -1, 0, 1);
    output.Append(element);

    output.RestartStrip();

    element.position = float4(-1, 1, 0, 1);
    output.Append(element);

    element.position = float4(1, 1, 0, 1);
    output.Append(element);

    element.position = float4(1, -1, 0, 1);
    output.Append(element);
}