#include "Octree.hlsl"

[maxvertexcount(16)]
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

    float4x4 worldInverse = transpose(WorldInverseTranspose);

    float3 cameraRight = float3(View[0][0], View[1][0], View[2][0]);
    float3 cameraUp = float3(View[0][1], View[1][1], View[2][1]);
    float3 cameraForward = float3(View[0][2], View[1][2], View[2][2]);

    // Transform the camera directions into local space
    cameraRight = normalize(mul(float4(cameraRight, 0), worldInverse));
    cameraUp = normalize(mul(float4(cameraUp, 0), worldInverse));
    cameraForward = normalize(mul(float4(cameraForward, 0), worldInverse));

    // Billboard should face in the same direction as the normal
    float3 up = 0.5f * input[0].size * normalize(cross(input[0].normal, cameraRight));
    float3 right = 0.5f * input[0].size * normalize(cross(input[0].normal, up));

    float4x4 WVP = mul(World, mul(View, Projection));

    GS_OUTPUT element;
    element.color = input[0].color;

    element.position = mul(float4(input[0].position + up - right, 1), WVP);
    output.Append(element);

    element.position = mul(float4(input[0].position - up + right, 1), WVP);
    output.Append(element);

    element.position = mul(float4(input[0].position - up - right, 1), WVP);
    output.Append(element);

    output.RestartStrip();

    element.position = mul(float4(input[0].position + up - right, 1), WVP);
    output.Append(element);

    element.position = mul(float4(input[0].position + up + right, 1), WVP);
    output.Append(element);

    element.position = mul(float4(input[0].position - up + right, 1), WVP);
    output.Append(element);
}