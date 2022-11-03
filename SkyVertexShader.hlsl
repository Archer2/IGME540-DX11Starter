#include "VertexInput.hlsli"

// Struct representing data passed down the pipeline - MUST MATCH SkyPixelShader!!
struct VertexToPixelSky {
	float4 position : SV_POSITION;
	float3 direction : DIRECTION;
};

// Constant data required for rendering the Sky
cbuffer SkyVertexData : register(b0)
{
	matrix c_viewMatrix;
	matrix c_projectionMatrix;
};

// Calculates Positional and Directional data from the provided input
VertexToPixelSky main(VertexShaderInput input)
{
	// Output to fill out
	VertexToPixelSky output;

	// Remove Translation
	matrix alteredView = c_viewMatrix;
	alteredView._14 = 0;
	alteredView._24 = 0;
	alteredView._34 = 0;

	// Set output position with camera matrices (no translation) and a perspective divide of 1
	output.position = mul(c_projectionMatrix, mul(alteredView, float4(input.localPosition, 1.f)));
	output.position.z = output.position.w;

	// Output direction is just input position, since each vertex position is just the offset
	// from the origin (because there is no translation)
	output.direction = normalize(input.localPosition); // Direction is 0-1

	return output;
}