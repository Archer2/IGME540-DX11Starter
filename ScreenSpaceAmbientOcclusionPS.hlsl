#include "ShaderHelpers.hlsli"

// All external data needed for this shader
cbuffer SSAOData : register(b0)
{
	matrix c_viewMatrix; // Active Scene Camera's View
	matrix c_projectionMatrix; // From active scene Camera
	matrix c_inverseProjMatrix; // Inverse of c_projectionMatrix
	float4 c_offsets[64]; // Random offset vectors
	float c_radius; // Distance to search for occluders
	int c_samples; // No more than size of c_offsets
	float2 c_randomSampleScreenScale; // window dimensions / randomTexture dimensions
};

Texture2D SceneNormals : register(t0);
Texture2D SceneDepths : register(t1);
Texture2D Random : register(t2);

SamplerState BasicSampler : register(s0);
SamplerState ClampSampler : register(s1);

// Calculate View space coordinate from a screen UV and depth value, using
// the scene camera's inverse Projection matrix
float3 ViewSpaceFromDepth(float a_depth, float2 a_uv)
{
	float2 ndc = a_uv * 2.f - 1.f;
	ndc.y *= -1.f; // Turn [0,1] UV range into NDC

	float4 screenPos = float4(ndc, a_depth, 1.f);
	float4 viewPos = mul(c_inverseProjMatrix, screenPos);

	return viewPos.xyz / viewPos.w; // Reverse the Perspective Divide
}

// Take a View Space position and get the screen UV (NOT NDC) of that
// position
float2 UVFromViewSpacePosition(float3 a_viewPos)
{
	float4 screenPos = mul(c_projectionMatrix, float4(a_viewPos, 1.f));
	screenPos.xyz /= screenPos.w; // Perspective Divide
	
	float2 uv = screenPos.xy * 0.5f + 0.5f; // Compress NDC to UV
	uv.y = 1.f - uv.y;

	return uv;
}

// Perform SSAO base calculation for each pixel
//	- It should be possible to get rid of random offsets and texture if the GPU
//	  rand() functions from ray tracing are brought over, though they may not be
//	  good enough(?)
float4 main(VertexToPixelFullscreenTriangle input) : SV_TARGET
{
	float pixelDepth = SceneDepths.Sample(ClampSampler, input.UV).r;
	if (pixelDepth == 1.f) return float4(1, 1, 1, 1); // Early out for skybox/background
	
	float3 pixelPosViewSpace = ViewSpaceFromDepth(pixelDepth, input.UV);

	// Assumes random texture holds normalized Vector3s
	float3 randomDir = Random.Sample(BasicSampler, input.UV * c_randomSampleScreenScale).xyz;

	float3 normal = SceneNormals.Sample(BasicSampler, input.UV).xyz * 2.f - 1.f; // Unpack Vector
	normal = normalize(mul((float3x3)c_viewMatrix, normal)); // Properly rotate normal

	float3 tangent = normalize(randomDir - normal * dot(randomDir, normal));
	float3 biTangent = cross(tangent, normal); // Normalized, since both vectors are
	float3x3 TBN = float3x3(tangent, biTangent, normal);

	float totalAO = 0.f;
	for (int i = 0; i < c_samples; i++) {
		float3 samplePosView = pixelPosViewSpace + mul(c_offsets[i].xyz, TBN) * c_radius;
		float2 sampleUV = UVFromViewSpacePosition(samplePosView);

		float sampleDepth = SceneDepths.SampleLevel(ClampSampler, sampleUV, 0).r;
		float sampleZ = ViewSpaceFromDepth(sampleDepth, sampleUV).z;

		// Compare Dpeths and fade rsults based on range (far away objects do not
		// increase occlusion from ambient light)
		float rangeCheck = smoothstep(0.f, 1.f, c_radius / abs(pixelPosViewSpace.z - sampleZ));
		totalAO += (sampleZ < samplePosView.z) ? rangeCheck : 0.f;
	}

	totalAO = 1.f - totalAO / c_samples; // Average results and flip
	return float4(totalAO.rrr, 1.f); // Greyscale image is best for debugging and visualizing
}