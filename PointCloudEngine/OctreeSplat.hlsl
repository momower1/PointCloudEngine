#include "OctreeSplatGS.hlsl"

VS_OUTPUT VS(VS_INPUT input)
{
    return VertexShaderFunction(input);
}

float4 PS(GS_OUTPUT input) : SV_TARGET
{
    return PixelShaderFunction(input);
}