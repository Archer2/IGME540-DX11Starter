/**
* Compute Pass to replicate Screen Space Ambient Occlusion computation
* Using a compute pass may actually be slower, because of hardware accelerations
* writing to textures from pixel shaders. Untested on my end, but I've read
* some things that hint at that
*/

// All external data needed for this shader
cbuffer SSAOData : register(b0)
{
	matrix c_viewMatrix; // Active Scene Camera's View
	matrix c_projectionMatrix; // From active scene Camera
	matrix c_inverseProjMatrix; // Inverse of c_projectionMatrix
	float4 c_offsets[64]; // Random offset vectors - This is a good opportunity for a StructuredBuffer
	float c_radius; // Distance to search for occluders
	int c_samples; // No more than size of c_offsets
	int2 c_windowDimensions;
	float2 c_randomSampleScreenScale; // window dimensions / randomTexture dimensions
};

// Read-in textures from scene
//	- No sampler is required, since textures will be indexed directly
Texture2D SceneNormals : register(t0);
Texture2D SceneDepths : register(t1);
Texture2D Random : register(t2);

// Output texture
RWTexture2D<unorm float4> SSAO : register(u0);

// Samplers
SamplerState BasicSampler : register(s0);
SamplerState ClampSampler : register(s1);

// Calculate View space coordinate from a screen UV and depth value, using
// the scene camera's inverse Projection matrix
//	- Must still use UV to accomodate for UV of offset vector location projections
float3 ViewSpaceFromDepth(float a_depth, float2 a_uv)
{
	float2 ndc = a_uv * 2.f - 1.f; // [0,1]->[-1,1]
	ndc.y *= -1.f; // Flip Y so - is down

	float4 screenPos = float4(ndc, a_depth, 1.f);
	float4 viewPos = mul(c_inverseProjMatrix, screenPos);

	return viewPos.xyz / viewPos.w; // Reverse the Perspective Divide
}

// Take a View Space position and get the screen UV (NOT NDC) of that
// position
//	- This still is acceptable to return UV values, because the provided
//	  position may not be exactly on a pixel and thus the int2 would be incorrect
float2 UVFromViewSpacePosition(float3 a_viewPos)
{
	float4 screenPos = mul(c_projectionMatrix, float4(a_viewPos, 1.f));
	screenPos.xyz /= screenPos.w; // Perspective Divide

	float2 uv = screenPos.xy * 0.5f + 0.5f; // Compress NDC to UV
	uv.y = 1.f - uv.y;

	return uv;
}

// Compute function to replicate SSAO calculation using Compute Shaders rather than
// a rasterization of a single triangle
//	- Runs in threadgroups of 32. A strong saturation of GPU cores is desired, and the maximum
//	  threadgroup size in CS Shader Model 5.0 is 1024 (regardless of hardware ability)
[numthreads(32, 32, 1)] // product of all dimensions must be <= 1024
void main( uint3 DTid : SV_DispatchThreadID )
{
	// Early exit on thread IDs that go above screen dimensions (assuming 0-based indexing)
	if ((int)DTid.x >= c_windowDimensions.x || (int)DTid.y >= c_windowDimensions.y) return;

	float pixelDepth = SceneDepths[DTid.xy].r; // Direct pixel access, not based on texture coordinates

	// Sky/Background should be white AO (not occluded at all)
	if (pixelDepth == 1.f) {
		SSAO[DTid.xy] = float4(1.f, 1.f, 1.f, 1.f);
		return;
	}

	float2 uv = float2(DTid.xy) / float2(c_windowDimensions); // Actual UV of this thread id

	float3 pixelPosViewSpace = ViewSpaceFromDepth(pixelDepth, uv);

	// Assumes random texture holds normalized Vector3s
	float3 randomDir = Random.SampleLevel(BasicSampler, uv * c_randomSampleScreenScale, 0).xyz;

	float3 normal = SceneNormals[DTid.xy].xyz * 2.f - 1.f; // Unpack Vector
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

		// Compare Depths and fade rsults based on range (far away objects do not
		// increase occlusion from ambient light)
		float rangeCheck = smoothstep(0.f, 1.f, c_radius / abs(pixelPosViewSpace.z - sampleZ));
		totalAO += (sampleZ < samplePosView.z) ? rangeCheck : 0.f;
	}

	totalAO = 1.f - totalAO / c_samples; // Average results and flip
	SSAO[DTid.xy] = float4(totalAO.rrr, 1.f);
}