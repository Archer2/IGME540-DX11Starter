#include "ShaderHelpers.hlsli"

// Specular G
// http://graphicrants.blogspot.com/2013/08/specular-brdf-reference.html
// https://github.com/vixorien/ggp-advanced-demos/blob/main/Image-Based%20Lighting%20for%20Indirect%20PBR/IBLBrdfLookUpTablePS.hlsl
// GeometricShadowing() function in ShaderHelpers.hlsli cannot be used,
// because it has a slightly different roughness adjustment
float G1_Schlick(float Roughness, float NdotV)
{
	float k = Roughness * Roughness;
	k /= 2.0f; // Schlick-GGX version of k - Used in UE4

	// Staying the same
	return NdotV / (NdotV * (1.0f - k) + k);
}

// Specular G
// http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf
// https://github.com/vixorien/ggp-advanced-demos/blob/main/Image-Based%20Lighting%20for%20Indirect%20PBR/IBLBrdfLookUpTablePS.hlsl
float G_Smith(float Roughness, float NdotV, float NdotL)
{
	return G1_Schlick(Roughness, NdotV) * G1_Schlick(Roughness, NdotL);
}

//----------------------------------------------------------
// Calculate the roughness:NdotV integral approximation. Code
// from Epic Games (2013)
// http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf
//----------------------------------------------------------
float2 IntegrateBRDF(float roughness, float NdotV)
{
	float3 V;
	V.x = sqrt(1.0f - NdotV * NdotV); // sin
	V.y = 0;
	V.z = NdotV; // cos

	float A = 0;
	float B = 0;

	float3 N = float3(0, 0, 1); // Not included in Epic's notes, but a Normal of +Z seems to be sufficient

	for (uint i = 0; i < MAX_IBL_SAMPLES; i++) {
		float2 Xi = Hammersley2d(i, MAX_IBL_SAMPLES);
		float3 H = ImportanceSampleGGX(Xi, roughness, N);
		float3 L = 2 * dot(V, H) * H - V;

		float NdotL = saturate(L.z);
		float NdotH = saturate(H.z);
		float VdotH = saturate(dot(V, H));

		if (NdotL > 0) {
			float G = G_Smith(roughness, NdotV, NdotL);
			float G_Vis = G * VdotH / (NdotH * NdotV);
			float Fc = pow(1 - VdotH, 5);
			A += (1 - Fc) * G_Vis;
			B += Fc * G_Vis;
		}
	}

	return float2(A, B) / MAX_IBL_SAMPLES;
}


//----------------------------------------------------------
// Take the UV for this pixel, treat as a roughness and
// normalDotView value, and calculate the BRDF integral for that
// pair. This is used for Specular IBL prefiltering so that values
// can be pulled out of the map and used to adjust the reflection
// color. The resulting texture is universal for all objects/environments
//----------------------------------------------------------
float4 main(VertexToPixelFullscreenTriangle input) : SV_TARGET
{
	// Treat UV as a grid of roughness to NdotV values
	float NdotV = input.UV.x; // NdotV is X
	float roughness = input.UV.y; // Roughness is Y

	float2 brdf = IntegrateBRDF(roughness, NdotV); // Calculate brdf for this input

	return float4(brdf, 0, 1); // final value is just rg
}