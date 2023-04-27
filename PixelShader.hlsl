#include "ShaderHelpers.hlsli"

#define MAX_LIGHTS_OF_SINGLE_TYPE 64

// Struct defining MRT output
struct PixelOutputs 
{
	float4 SceneColor : SV_TARGET0;
	float4 SceneAmbient : SV_TARGET1;
	float4 SceneNormal : SV_TARGET2;
	float SceneDepth : SV_TARGET3; // Render Target at slot 3 MUST have single-value format
};

// Struct representing constant data shared between all pixels
cbuffer PixelConstantData : register(b0)
{
	float4 c_color; // Color to tint the main color with
	float3 c_cameraPosition; // Position of the active Camera
	float  c_roughnessScale; // Inverse shininess of the object
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
TextureCube ReflectionMap : register(t5); // IBL Specular Reflection Map (1/2 Split Sum Approximation)
Texture2D BRDFIntegrationMap : register(t6); // IBL Specular Reflection BRDF Lookup Table

SamplerState BasicSampler : register(s0); // s registers for samplers
SamplerState ClampSampler : register(s1); // Clamp address mode is required for Specular IBL reference map sampling

// --------------------------------------------------------
// Approximate the Specular IBL Lighting. Stored as a function
// here for easier access to texture resources. Code referenced
// from Epic Games (2013, 
// https://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf)
// and Chris Cascioli for roughness usage and map sampling
// (https://github.com/vixorien/ggp-advanced-demos/blob/main/Image-Based%20Lighting%20for%20Indirect%20PBR/Lighting.hlsli)
// --------------------------------------------------------
float3 SpecularIBLApproximation(float3 specColor, float roughness, float3 N, float3 pixelToCam)
{
	float3 R = normalize(reflect(-pixelToCam, N));
	float NdotV = saturate(dot(N, pixelToCam)); // Dot product Normal . View
	//float3 R = 2 * dot(V, N) * N - V; // Reflection Vector calculation from Epic. Not used here, since map is pre-filtered
										// Would be fed into IBLSpecularPrefilterPS.hlsl::PrefilterEnvMap as R
	roughness = max(roughness, MIN_ROUGHNESS); // Ensure roughness is usable

	// Sample maps 
	// 6 is a placeholder for cbuffer data - Number of mips in Convolved Map
	// Reverse gamma correction from ONLY reflection map - it is factored back in during 
	// lighting calculation. IBL textures do not have this built in, so it must be taken out before 
	float3 reflectionColor = pow(abs(ReflectionMap.SampleLevel(ClampSampler, R, roughness * 6).rgb), 2.2f);
	float2 brdf = BRDFIntegrationMap.Sample(ClampSampler, float2(NdotV, roughness)).rg;

	return reflectionColor * (specColor * brdf.x + brdf.y); // Modify reflectionColor by BRDF Fresnel
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
#if MULTIPLE_RENDER_TARGETS
PixelOutputs main(VertexToPixel input)
#else
float4 main(VertexToPixel input) : SV_TARGET
#endif
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
	roughnessValue *= 1.f - c_roughnessScale; // This is in place to make demoing IBL textures easier, removing the need for a material/texture per roughness level
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
	//indirectDiffuse = ConserveDiffuseEnergy(indirectDiffuse, specularColor, metalnessValue);

	//return ReflectionMap.SampleLevel(BasicSampler, input.normal, roughnessValue * 5); // TESTING to display IBLSpecMap
	float3 indirectSpecular = SpecularIBLApproximation(specularColor, roughnessValue, input.normal, cameraVector);
	//return float4(indirectSpecular, 1); TESTING - display only indirect specular
	indirectDiffuse = ConserveDiffuseEnergy(indirectDiffuse, indirectSpecular, metalnessValue);

	float3 indirectSum = indirectSpecular + (indirectDiffuse * albedoColor);

	// TEST Indirect Diffuse Lighting
	//return float4(pow(indirectDiffuse, 1.0f / 2.2f), 1.f);
	//return float4(pow(indirectSum, 1.0f / 2.2f), 1.f);

	// Calculate and return final light
	float3 finalLight = directionalLightSum + pointLightSum + indirectSum;

#if MULTIPLE_RENDER_TARGETS
	PixelOutputs output;
	output.SceneColor = float4(directionalLightSum + pointLightSum + indirectSpecular, 1.f); // NO Gamma Correction!
	output.SceneAmbient = float4(albedoColor * indirectDiffuse, 1.f); // "Ambient" is diffuse indirect light - NO Gamma Correction!
	output.SceneNormal = float4(input.normal * .5f + .5f, 1.f); // Pack into 0-1 range
	output.SceneDepth = input.screenPosition.z;
	return output;
#else
	return float4(pow(finalLight, 1.0f/2.2f), 1.f); // Apply Gamma Correction, and set the Alpha value to 1 (since it doesn't matter)
#endif
}