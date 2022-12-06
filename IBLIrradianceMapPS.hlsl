// Pixel Shader for calculating the IBL Diffuse Irradiance Map

// PI constants
static const float PI = 3.14159265359f;
static const float TWO_PI = PI * 2.0f;
static const float PI_OVER_2 = PI / 2.0f;

// A draw call must be initiated for all 6 faces of the Environment Map. This PS should
// run once for every pixel in that face of the Environment Map. The "Normal" of the pixel
// can be determined by the UV: Unpack to [-1,1], then treat it as a plane 1 unit away
// from an "origin point" (all Z values of the UV plane are 1). The direction from the
// origin to the pixel is the normalized 3d vector created from the UV and Z (1). The
// components are shuffled before normalizing based on the Face orientation
cbuffer irradianceData : register(b0)
{
	int c_face;
	float c_phiStep;
	float c_thetaStep;
}

// Position is required data for the pipeline, UV is required for Irradiance Map
struct VertexToPixel 
{
	float4 Position : SV_POSITION;
	float2 UV		: TEXCOORD;
};

// Environment Map and Sampler
TextureCube EnvMap : register(t0);
SamplerState Sampler : register(s0);

// Calculate the Irradiance Map for the provided Environment Map
// This is done by calculating the direction and tangent of the pixel from the input
// UV, then iterating around a hemisphere pointed in that direction in an approximation
// of a double-integral on two angles. The approximation is the average of all colors
// sampled at each step of the iteration
//	- Sources for the math and code for this include:
//	  http://www.codinglabs.net/article_physically_based_rendering.aspx
//	  https://www.learnopengl.com/PBR/IBL/Diffuse-irradiance
//	  https://www.github.com/vixorien/ggp-advanced-demos/tree/main/Image-Based%20Lighting%20for%20Indirect%20PBR
//		- Particularly the files "Lighting.hlsli", "PixelShaderPBR.hlsl", "IBLIrradianceMapPS.hlsl"
//	- All of the above sources represent phi as the 360deg angle around the base of the hemisphere, and
//	  theta as the 0-90deg angle down, but in my head it made more sense if the two were flipped (with the
//	  hemisphere pointing towards me, instead of up), so that's the way I wrote the angles
float4 main(VertexToPixel input) : SV_TARGET
{
	float2 uv = input.UV * 2.f - 1.f; // Unpack to [1,-1] range for a plane
	float3 direction = float3(uv.x, -uv.y, 1.f);

	// Switch on the Face index to rearrange Normal components
	// to be appropriate to that face (default is FRONT (+Z))
	float tmp;
	switch (c_face) {
	case 0: // +X, z becomes -x, x becomes z
		//tmp = direction.z;
		//direction.z = -direction.x;
		//direction.x = tmp;
		direction = float3(1.f, -uv.y, -uv.x);
		break;
	case 1: // -X, z becomes x, x becomes -z
		//tmp = -direction.z;
		//direction.z = direction.x;
		//direction.x = tmp;
		direction = float3(-1.f, -uv.y, uv.x);
		break;
	case 2: // +Y, z becomes -y, y becomes z
		//tmp = direction.z;
		//direction.z = -direction.y;
		//direction.y = tmp;
		direction = float3(uv.x, 1.f, uv.y);
		break;
	case 3: // -Y, z becomes y, y becomes -z
		//tmp = -direction.z;
		//direction.z = direction.y;
		//direction.y = tmp;
		direction = float3(uv.x, -1.f, -uv.y);
		break;
	case 4: // +Z, already ordered as such
		direction = float3(uv.x, -uv.y, 1.f);
		break;
	case 5: // -Z, z becomes -z
		//direction.z = -direction.z;
		//direction.x = -direction.x;
		direction = float3(-uv.x, -uv.y, -1.f);
		break;
	default: return float4(1.f, 1.f, 1.f, 1.f); // Error color returned
	}
	direction = normalize(direction); // normal at this point has magnitude 1
	float3 tangent = cross(float3(0, 1, 0), direction); // X-direction, should already be normalized
	float3 bitangent = cross(direction, tangent); // Y-direction, should be normalized

	float3 totalColor = float3(0.f, 0.f, 0.f);
	int samples = 0;
	float sinTheta, cosTheta;
	float sinPhi, cosPhi;

	// Loop around each angle of the hemisphere, using spherical coordinates to identify a
	// "position" at radius 1. This can be used as a direction by treating the hemisphere as
	// centered at 0. 
	//	- The final "pixel" direction is 'normalize(hemisphericalCoords + direction)'
	//		- Not quite - the x and y direction of tangent space must be accounted for, similar to
	//		  normal map unwrapping
	for (float theta = 0.0f; theta < PI * 2.f; theta += c_thetaStep) 
	{
		sincos(theta, sinTheta, cosTheta); // theta is the angle around the hemisphere's base, 0-360deg

		// PHI starts at 0 and goes to 90 degrees, with 0 being a light direction pointing directly
		// at the normal and 90 being a light direction perpendicular to the normal, where normal is
		// the hemisphere direction
		for (float phi = 0.0f; phi < PI / 2.f; phi += c_phiStep) 
		{
			sincos(phi, sinPhi, cosPhi); // phi is the angle 0-90deg down the hemisphere from the hemisphere direction
			
			// Spherical coordinates, r = 1
			float3 hemisphereCoords = normalize(float3(cosTheta * sinPhi, sinTheta * sinPhi, cosPhi));
			float3 iterDir = hemisphereCoords.x * tangent + hemisphereCoords.y * bitangent + hemisphereCoords.z * direction;
				//normalize(hemisphereCoords + direction); //<--- This causes the map to be a little brighter, because no tangent info?
			
			// Sample EnvMap, reverse embedded gamma correction, scale by cos(phi) for NdotL diffusion, scale by sin(phi) because more
			// samples are taken at the samller angles of the hemisphere and that effect needs to be scaled out
			totalColor += pow(EnvMap.Sample(Sampler, iterDir).rgb, 2.2) * cos(phi) * sin(phi);
			samples++;
		}
	}

	totalColor = PI * totalColor / samples; // Average the total color - PI is a constant multiplier outside of the double-angle integral
	return float4(pow(totalColor, 1.f / 2.2f), 1.f); // Apply gamma correction to the final
}