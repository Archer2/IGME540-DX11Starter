#include "ShaderHelpers.hlsli"

#define DIRECTIONAL_LIGHT_COUNT 3
#define POINT_LIGHT_COUNT 2

// Struct representing constant data shared between all pixels
cbuffer PixelConstantData : register(b0)
{
	float4 c_color; // Color to tint the main color with
	float3 c_cameraPosition; // Position of the active Camera
	float  c_roughness; // Inverse shininess of the object
	float2 c_uvOffset; // Universal Offset for the UVs
	float  c_uvScale; // Universal Scale for the UVs
}

// Struct representing constant lighting data (ambient color and generic light) for all pixels
cbuffer PixelLightingData : register(b1)
{
	float4 c_ambientLight; // Scene ambient color
	Light c_directionalLights[DIRECTIONAL_LIGHT_COUNT]; // Sample directional lights
	Light c_pointLights[POINT_LIGHT_COUNT]; // Sample point lights
}

// Textures and Samplers
Texture2D DiffuseTexture : register(t0); // t registers for textures
Texture2D NormalTexture : register(t1); // Normal map
Texture2D RoughnessTexture : register(t2); // Inverse of a Specular Map, because that is just the textures I downloaded

SamplerState BasicSampler : register(s0); // s registers for samplers

// --------------------------------------------------------
// The entry point (main method) for our pixel shader
// 
// - Input is the data coming down the pipeline (defined by the struct)
// - Output is a single color (float4)
// - Has a special semantic (SV_TARGET), which means 
//    "put the output of this into the current render target"
// - Named "main" because that's the default the shader compiler looks for
// --------------------------------------------------------
float4 main(VertexToPixel input) : SV_TARGET
{
	// Re-normalize input
	input.normal = normalize(input.normal);
	float T = normalize(input.tangent);
	input.tangent = normalize(T - input.normal * dot(T, input.normal)); // Graham-Schmidt ortho-normalization
	
	// Modify UVs (scale first, then offset)
	input.uv /= c_uvScale; // Invert the scale so that .1 actually multiplies UV by 10 instead of dividing
	input.uv += c_uvOffset;

	// Calculate basic color modifiers from Textures (surface color and specular modifier)
	float4 materialColor = float4((c_color * DiffuseTexture.Sample(BasicSampler, input.uv)).rgb, 1.f); // normalize A to 1
	float specValue = 1.f - RoughnessTexture.Sample(BasicSampler, input.uv).r; // Textures used are Roughness values, not Specular, so invert

	// Calculate Normal modifier, and assign to input value for ease of lighting
	float3 unpackedNormal = NormalTexture.Sample(BasicSampler, input.uv).rgb * 2 - 1; // [0, 1] -> [-1, 1]
	float3 biTangent = cross(input.tangent, input.normal);
	float3x3 TBN = float3x3(input.tangent, biTangent, input.normal);
	input.normal = mul(unpackedNormal, TBN);

	// Calculate Ambient Light
	float4 ambientTerm = c_ambientLight * materialColor;
	
	// Sum Directional Light calculations
	float4 directionalLightSum = float4(0.f, 0.f, 0.f, 1.f);
	for (int i = 0; i < DIRECTIONAL_LIGHT_COUNT; i++) {
		directionalLightSum += CalculateDirectionalLightDiffuseAndSpecular(
			c_directionalLights[i], input, c_cameraPosition, c_roughness, materialColor, specValue);
	}

	// Sum Point Light calculations
	float4 pointLightSum = float4(0.f, 0.f, 0.f, 1.f);
	for (int j = 0; j < POINT_LIGHT_COUNT; j++) {
		pointLightSum += CalculatePointLightDiffuseAndSpecular(
			c_pointLights[j], input, c_cameraPosition, c_roughness, materialColor, specValue);
	}

	return ambientTerm + directionalLightSum + pointLightSum;
}