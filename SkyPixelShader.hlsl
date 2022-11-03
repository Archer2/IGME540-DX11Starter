
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
	return CubeMap.Sample(SkySampler, input.direction);
}