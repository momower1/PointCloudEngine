cbuffer SplatRendererConstantBuffer : register(b0)
{
    float4x4 World;
//------------------------------------------------------------------------------ (64 byte boundary)
    float4x4 View;
//------------------------------------------------------------------------------ (64 byte boundary)
    float4x4 Projection;
//------------------------------------------------------------------------------ (64 byte boundary)
    float4x4 WorldInverseTranspose;
//------------------------------------------------------------------------------ (64 byte boundary)
    float3 cameraPosition;
    float fovAngleY;
//------------------------------------------------------------------------------ (16 byte boundary)
    float splatSize;
    // 12 bytes auto padding
//------------------------------------------------------------------------------ (16 byte boundary)
};  // Total: 288 bytes with constant buffer packing rules

struct VS_INPUT
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    uint3 color : COLOR;
};

struct VS_OUTPUT
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float3 color : COLOR;
};

struct GS_OUTPUT
{
	float4 position : SV_POSITION;
	float3 positionWorld : POSITION2;
	float3 positionCenter : POSITION3;
	float3 color : COLOR;
	float radius : RADIUS;
};

VS_OUTPUT VS(VS_INPUT input)
{
    VS_OUTPUT output;
    output.position = mul(float4(input.position, 1), World);
    output.normal = normalize(mul(input.normal, WorldInverseTranspose));
    output.color = input.color / 255.0f;

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

    float3 cameraRight = float3(View[0][0], View[1][0], View[2][0]);
    float3 cameraUp = float3(View[0][1], View[1][1], View[2][1]);
    float3 cameraForward = float3(View[0][2], View[1][2], View[2][2]);

    // Billboard should face in the same direction as the normal
    float distanceToCamera = distance(cameraPosition, input[0].position);
	float splatSizeWorld = length(mul(float3(splatSize, 0, 0), World).xyz);
    float3 up = 0.5f * splatSizeWorld * normalize(cross(input[0].normal, cameraRight));
    float3 right = 0.5f * splatSizeWorld * normalize(cross(input[0].normal, up));

    float4x4 VP = mul(View, Projection);

    GS_OUTPUT element;
	element.positionCenter = input[0].position;
    element.color = input[0].color;
	element.radius = length(up);

	// Append all the vertices in the correct order to create the billboard
	element.positionWorld = input[0].position + up - right;
    element.position = mul(float4(element.positionWorld, 1), VP);
    output.Append(element);

	element.positionWorld = input[0].position - up + right;
    element.position = mul(float4(element.positionWorld, 1), VP);
    output.Append(element);

	element.positionWorld = input[0].position - up - right;
    element.position = mul(float4(element.positionWorld, 1), VP);
    output.Append(element);

    output.RestartStrip();

	element.positionWorld = input[0].position + up - right;
    element.position = mul(float4(element.positionWorld, 1), VP);
    output.Append(element);

	element.positionWorld = input[0].position + up + right;
    element.position = mul(float4(element.positionWorld, 1), VP);
    output.Append(element);

	element.positionWorld = input[0].position - up + right;
    element.position = mul(float4(element.positionWorld, 1), VP);
    output.Append(element);
}

float4 PS(GS_OUTPUT input) : SV_TARGET
{
	// Make circular splats, remove this for squares
	if (distance(input.positionWorld, input.positionCenter) > input.radius)
	{
		discard;
	}

    return float4(input.color, 1);
}