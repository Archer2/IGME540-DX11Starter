#include "ShaderHelpers.hlsli"

Texture2D SceneColors : register(t0);
Texture2D SceneAmbient : register(t1);
Texture2D SceneDepths : register(t2); // Used to test for Sky to not gamma correct that
Texture2D SSAO : register(t3); // Should be blurred

SamplerState ClampSampler : register(s0);

// Combine rendered scene with SSAO pass to occlude indirect light
float4 main(VertexToPixelFullscreenTriangle input) : SV_TARGET
{
	float3 sceneColor = SceneColors.Sample(ClampSampler, input.UV).rgb;
	float3 ambient = SceneAmbient.Sample(ClampSampler, input.UV).rgb;
	float ao = SSAO.Sample(ClampSampler, input.UV).r;

	// Check against depth to determine if this pixel is in the sky. If it is, do not gamma
	// correct, since the sky already has that baked into those pixel colors from the texture
	// sample
	//	- I chose this method to avoiding modifying a large number of other Pixel shaders -
	//	  not everything should have to change to accomodate 1 post process
	//	- This may not be ideal, since there could be objects at high depth that do not get proper
	//	  gamma correction, and it does add a resource and texture sample to this shader
	//		- There is a slight frame dip of 2-3fps when very close to objects
	float3 color;
	if (SceneDepths.Sample(ClampSampler, input.UV).r >= 0.99999f) // Use a small epsilon
		color = ambient * ao + sceneColor;
	else
		color = pow(abs(ambient * ao + sceneColor), 1.f / 2.2f);

	// Universally applying gamma correction doesn't work very well, since it will also
	// correct the Sky. Since the Sky is just a directly sampled texture that already has
	// gamma correction baked in, this makes it look washed out
	return float4(color, 1.f);

}