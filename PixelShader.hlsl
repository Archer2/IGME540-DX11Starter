#include "ShaderHelpers.hlsli"

#define DIRECTIONAL_LIGHT_COUNT 3
#define POINT_LIGHT_COUNT 2

// Struct representing constant data shared between all pixels
cbuffer PixelConstantData : register(b0)
{
	float4 c_color; // Color to tint the main color with
	float3 c_cameraPosition; // Position of the active Camera
	float  c_roughness; // Inverse shininess of the object
}

cbuffer PixelLightingData : register(b1)
{
	float4 c_ambientLight; // Scene ambient color
	Light c_directionalLights[DIRECTIONAL_LIGHT_COUNT]; // Sample directional lights
	Light c_pointLights[POINT_LIGHT_COUNT]; // Sample point lights
}

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
	// Re-normalize input normal
	input.normal = normalize(input.normal);
	
	// Calculate Ambient Light
	float4 ambientTerm = c_ambientLight * c_color;
	
	// Sum Directional Light calculations
	float4 directionalLightSum = float4(0.f, 0.f, 0.f, 1.f);
	for (int i = 0; i < DIRECTIONAL_LIGHT_COUNT; i++) {
		directionalLightSum += CalculateDirectionalLightDiffuseAndSpecular(
			c_directionalLights[i], input, c_cameraPosition, c_roughness, c_color);
	}

	// Sum Point Light calculations
	float4 pointLightSum = float4(0.f, 0.f, 0.f, 1.f);
	for (int j = 0; j < POINT_LIGHT_COUNT; j++) {
		pointLightSum += CalculatePointLightDiffuseAndSpecular(
			c_pointLights[j], input, c_cameraPosition, c_roughness, c_color);
	}

	return ambientTerm + directionalLightSum + pointLightSum;
}