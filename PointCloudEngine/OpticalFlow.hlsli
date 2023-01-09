float3 HUEtoRGB(in float H)
{
    float R = abs(H * 6 - 3) - 1;
    float G = 2 - abs(H * 6 - 2);
    float B = 2 - abs(H * 6 - 4);
    return saturate(float3(R, G, B));
}

float3 HSVtoRGB(in float3 HSV)
{
    float3 RGB = HUEtoRGB(HSV.x);
    return ((RGB - 1) * HSV.y + 1) * HSV.z;
}

float4 FlowToColor(in float2 motion)
{
    float motionColorScale = 0.05f;

    // Visualize the motion vector by mapping its angle to color and its length to saturation (no motion -> black)
    // Up           -> Green
    // Up Right     -> Orange
    // Right        -> Red
    // Down Right   -> Magenta
    // Down         -> Purple
    // Down Left    -> Blue
    // Left         -> Turquoise
    // Up Left      -> Lime
    float saturation = saturate(motionColorScale * length(motion));

    // Calculate signed angle and map to hue component
    float2 direction = normalize(motion);
    float signedAngle = atan2(direction.y, direction.x) - atan2(0.0f, 1.0f);
    float hue = 0.5f + 0.5f * (signedAngle / 3.141592654f);

    // Convert from HSV to RGB
    float3 hsv = float3(hue, saturation, 1.0f);
    float3 color = float3(1.0, 1.0, 1.0) - HSVtoRGB(hsv);

    return float4(color.r, color.g, color.b, 1.0f);
}