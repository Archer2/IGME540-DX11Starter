#include "ShaderHelpers.hlsli"

// Any external data required for the box blur
cbuffer BlurData : register(b0)
{
	float2 c_pixelSize; // 1 / windowWidth, 1 / windowHeight
};

Texture2D BlurTarget : register(t0);
SamplerState ClampSampler : register(s0);

// Perform a simple 4x4 box blur on the passed in image
float4 main(VertexToPixelFullscreenTriangle input) : SV_TARGET
{
	float blur = 0.f;
	for (float x = -1.5f; x <= 1.5f; x++) {
		for (float y = -1.5f; y <= 1.5f; y++) {
			blur += BlurTarget.Sample(ClampSampler, float2(x, y) * c_pixelSize + input.UV).r;
		}
	}
	blur /= 16.f; // Average from 16 samples
	return float4(blur.rrr, 1.f);
}