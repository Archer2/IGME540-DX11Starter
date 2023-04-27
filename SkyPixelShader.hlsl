#include "ShaderHelpers.hlsli" // Included for MRT flag

// Struct representing data passed down the pipeline - MUST MATCH SkyVertexShader!!
struct VertexToPixelSky {
	float4 position : SV_POSITION;
	float3 direction : DIRECTION;
};

// Cube Map texture and sampler
TextureCube CubeMap : register(t0);
SamplerState SkySampler : register(s0);

// Return the CubeMap color in the sampled direction
float4 main(VertexToPixelSky input) : SV_TARGET
{
	// Alternate method for fixing SSAO gamma correction in sky is to reverse what is baked
	// into the sky texture before rendering
//#if MULTIPLE_RENDER_TARGETS
//	return pow(abs(CubeMap.Sample(SkySampler, input.direction)), 2.2f);
//#else
	return CubeMap.Sample(SkySampler, input.direction);
//#endif
}