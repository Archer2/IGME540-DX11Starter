#include "ShaderHelpers.hlsli"

// Constant data for the specular environment map prefilter
// includes the Texture Cube face of the pixel and the roughness
// level of this cubemap
cbuffer prefilterData : register(b0)
{
	float c_roughness;
	int c_face;
}

// Environment Cubemap and Sampler
TextureCube EnvMap : register(t0);
SamplerState Sampler : register(s0);

// Prefilter the Environment Map at this reflection direction and roughness level.
// Requires a tremendous number of samples (samples * 6 * cube pixels)
//	- roughness - roughness level of this cube
//	- R - reflection vector (normalized)
float3 PrefilterEnvMap(float roughness, float3 R)
{
	// Normal = View = Reflectance Directions
	// While this assumption does result in a rather simple approximation, it is common
	// for performance and does not noticable reduce quality
	float3 N = R;
	float3 V = R;

	float3 totalColor = float3(0.f, 0.f, 0.f);
	float totalWeight = 0.f;

	// Perform a number of samples along the Hammersley point set. More is better,
	// but requires stronger hardware
	for (uint i = 0; i < MAX_IBL_SAMPLES; i++) {
		float2 Xi = Hammersley2d(i, MAX_IBL_SAMPLES);
		float3 H = ImportanceSampleGGX(Xi, roughness, N);
		float3 L = 2 * dot(V, H) * H - V;

		float NdotL = saturate(dot(N, L));
		if (NdotL > 0) {
			float3 sampleColor = EnvMap.SampleLevel(Sampler, L, 0).rgb;
			totalColor += pow(abs(sampleColor), 2.2) * NdotL; // Apply inverse gamma correction
			totalWeight += NdotL;
		}
	}
	
	// Average the result of the samples
	return pow(abs(totalColor / totalWeight), 1.f / 2.2f); // Apply gamma correction
}


// Specular Prefiltering is one half of the split-sum approximation,
// involving calculating the specular reflection cubemap at multiple roughness
// levels. This is stored as one cubmap with roughness levels as the mip levels
// 
float4 main(VertexToPixelFullscreenTriangle input) : SV_TARGET
{
	// Use the same trick as the Irradiance Map calculation to get the direction
	// of this pixel
	float3 direction = CubeDirectionFromUV(input.UV, c_face);

	float3 envColor = PrefilterEnvMap(c_roughness, direction);
	return float4(envColor, 1.f);
}