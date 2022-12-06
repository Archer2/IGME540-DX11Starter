#include "ShaderHelpers.hlsli"

#define MAX_LIGHTS_OF_SINGLE_TYPE 64

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
	Light c_directionalLights[MAX_LIGHTS_OF_SINGLE_TYPE]; // Sample directional lights
	Light c_pointLights[MAX_LIGHTS_OF_SINGLE_TYPE]; // Sample point lights
	int c_directionalLightCount; // 4-bytes from boundary
	int c_pointLightCount; // 8-bytes
}

// Textures and Samplers
Texture2D AlbedoTexture : register(t0); // PBR Albedo
Texture2D NormalTexture : register(t1); // Normal Map
Texture2D RoughnessTexture : register(t2); // PBR Roughness Map
Texture2D MetalnessTexture : register(t3); // PBR Metalness Map

TextureCube IrradianceMap : register(t4); // IBL Irradiance Map for Diffuse Light

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
	float3 T = normalize(input.tangent);
	input.tangent = normalize(T - input.normal * dot(T, input.normal)); // Graham-Schmidt ortho-normalization
	
	// Modify UVs (scale first, then offset)
	input.uv /= c_uvScale; // Invert the scale so that .1 actually multiplies UV by 10 instead of dividing
	input.uv += c_uvOffset;

	// Calculate basic color and modifiers from Textures
	float3 albedoColor = AlbedoTexture.Sample(BasicSampler, input.uv).rgb; // Alpha channel is unused here
	albedoColor = pow(albedoColor, 2.2f) * c_color.rgb; // Reverse embedded image Gamma Correction before lighting is applied (only done for surface color texture)

	float roughnessValue = RoughnessTexture.Sample(BasicSampler, input.uv).r; // Possibly should be saturated
	float metalnessValue = MetalnessTexture.Sample(BasicSampler, input.uv).r; // Probably should be saturated

	// For metals, specular is actually just the albedo. Lerp is used because filtering the texture can result in Metalness Values of not 0 or 1
	float3 specularColor = lerp(F0_NON_METAL.rrr, albedoColor, metalnessValue); // F0_NON_METAL defined in ShaderHelpers.h

	// Calculate Normal modifier, and assign to input value for ease of lighting
	float3 unpackedNormal = NormalTexture.Sample(BasicSampler, input.uv).rgb * 2 - 1; // [0, 1] -> [-1, 1]
	float3 biTangent = cross(input.normal, input.tangent);
	float3x3 TBN = float3x3(input.tangent, biTangent, input.normal);
	input.normal = mul(normalize(unpackedNormal), TBN);

	float3 cameraVector = normalize(c_cameraPosition - input.worldPosition); // Normalized vector from Pixel to Camera

	// Sum Directional Light calculations - PBR
	float3 directionalLightSum = float3(0.f, 0.f, 0.f);
	for (int i = 0; i < c_directionalLightCount; i++) {
		directionalLightSum += CalculateDirectionalLightDiffuseAndSpecular(
			c_directionalLights[i], input, cameraVector, roughnessValue, metalnessValue, specularColor, albedoColor);
	}

	// Sum Point Light calculations - PBR
	float3 pointLightSum = float3(0.f, 0.f, 0.f);
	for (int j = 0; j < c_pointLightCount; j++) {
		pointLightSum += CalculatePointLightDiffuseAndSpecular(
			c_pointLights[j], input, cameraVector, roughnessValue, metalnessValue, specularColor, albedoColor);
	}

	// Calculate IBL Light
	float3 indirectDiffuse = pow(abs(IrradianceMap.Sample(BasicSampler, input.normal).rgb), 2.2f);
	indirectDiffuse = ConserveDiffuseEnergy(indirectDiffuse, specularColor, metalnessValue);

	float3 indirectSum = indirectDiffuse * albedoColor;

	// Demo Indirect Diffuse Lighting
	//return float4(pow(indirectDiffuse, 1.0f / 2.2f), 1.f);
	//return float4(pow(indirectSum, 1.0f / 2.2f), 1.f);

	// Calculate and return final light
	float3 finalLight = directionalLightSum + pointLightSum + indirectSum;
	return float4(pow(finalLight, 1.0f/2.2f), 1.f); // Apply Gamma Correction, and set the Alpha value to 1 (since it doesn't matter)
}