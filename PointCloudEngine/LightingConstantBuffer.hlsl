cbuffer LightingConstantBuffer : register(b1)
{
	bool light;
	float3 lightDirection;
//------------------------------------------------------------------------------ (16 byte boundary)
	float lightIntensity;
	float ambient;
	float diffuse;
	float specular;
//------------------------------------------------------------------------------ (16 byte boundary)
	float specularExponent;
	// 12 byte auto padding
//------------------------------------------------------------------------------ (16 byte boundary)
};	// Total: 48 bytes with constant buffer packing rules

float4 PhongLighting(float3 cameraPosition, float3 position, float3 normal, float3 albedo)
{
	// Apply simple phong lighting
	float3 lightColor = float3(1, 1, 1);
	float3 n = normalize(normal);
	float3 l = normalize(-lightDirection);
	float3 v = normalize(cameraPosition - position);
	float3 r = reflect(-l, n);

	float diffuseIntensity = max(ambient, lightIntensity * diffuse * dot(n, l));
	float specularIntensity = lightIntensity * pow(max(0, specular * dot(r, v)), specularExponent);

	return float4(diffuseIntensity * albedo + specularIntensity * lightColor, 1);
}