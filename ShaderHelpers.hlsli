#ifndef _SHADER_COMMON_INCLUDES_
#define _SHADER_COMMON_INCLUDES_

#include "Hammersley.hlsli"

// Macros describing type integers for BasicLight Types
// - Must match LightType enum definition in Lights.h in C++
#define LIGHT_TYPE_DIRECTIONAL 0
#define LIGHT_TYPE_POINT 1
#define LIGHT_TYPE_SPOT 2

// Macro for the largest possible Specular Light exponent
#define MAX_SPECULAR_EXPONENT 256.0f

// Macro for the maximum number of samples taken during the calculation of a Specular IBL
// pre-filtered environment map
#define MAX_IBL_SAMPLES 1024 // 1024 for performance - 4096 ideal

// Constant floats necessary for Microfacet BRDF components
// The fresnel value for non-metals (dielectrics)
// Page 9: "F0 of nonmetals is now a constant 0.04"
// http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf
static const float F0_NON_METAL = 0.04f;
// Minimum roughness for when spec distribution function denominator goes to zero
static const float MIN_ROUGHNESS = 0.0000001f; // 6 zeros after decimal

static const float PI = 3.14159265359f;
static const float TWO_PI = PI * 2.0f;
static const float PI_OVER_2 = PI / 2.0f;

// Struct defining basic Light properties for all basic types
// - Must match BasicLight struct definition in Lights.h
// - A good way to remove branching is probably to create different
//	 structs, one for each Light only holding that Light's values.
//	 Then pass in an array of each type of Light
struct Light {
	int Type;			// Directional, Point, or Spot
	float3 Position;	// World Position - Point and Spot
	float Range;		// Attenuation Distance - Point and Spot
	float3 Direction;	// Normalized Direction - Directional and Spot
	float Intensity;	// Intensity Scalar - All Types
	float3 Color;		// Light Color - All Types
	float SpotAngle;	// Cone Angle - Spot
	float3 _padding;	// Line this struct up to a 16-byte multiple size
};

// Struct representing the data we're sending down the pipeline
// - Should match our pixel shader's input (hence the name: Vertex to Pixel)
// - At a minimum, we need a piece of data defined tagged as SV_POSITION
// - The name of the struct itself is unimportant, but should be descriptive
// - Each variable must have a semantic, which defines its usage
struct VertexToPixel
{
	// Data type
	//  |
	//  |   Name          Semantic
	//  |    |                |
	//  v    v                v
	float4 screenPosition	: SV_POSITION;	// XYZW position (System Value Position)
	float3 normal			: NORMAL;       // RGBA color
	float2 uv				: TEXCOORD;		// UV texture coordinate
	float3 worldPosition	: POSITION;		// World Position of the vertex to be interpolated
	float3 tangent			: TANGENT;		// Tangent of the vertex to be used in Normal Mapping
};

// Struct representing data sent down the pipeline for all Pixel Shaders using
// the full-screen Triangle trick to render images instead of geometry
struct VertexToPixelFullscreenTriangle 
{
	float4 Position : SV_POSITION;
	float2 UV		: TEXCOORD;
};

// Basic Diffuse Light calculation - Lambertian (N Dot L) Diffusion
//	- directionToLight: normalized direction from pixel to Light
//	- normal: normalized normal of pixel
float3 DiffuseBRDF(float3 directionToLight, float3 normal)
{
	return saturate(dot(normal, directionToLight));
}

// Phong Specular Light Calculation. WARNING - BRANCHING CODE - branches should all be uniform
//	- normal: normal of pixel
//	- pixelPosition: world position of the pixel
//	- cameraPosition: world position of the eyepoint
//	- lightDirection: direction of the Light - Light TO Pixel
//	- materialSpecular: shininess of the material
//	- pixelSpecular: specular value of this pixel from a SpecMap (1 to ignore)
float3 PhongBRDF(float3 normal, float3 pixelPosition, float3 cameraPosition, 
	float3 lightDirection, float materialSpecular, float pixelSpecular)
{
	float3 v = normalize(cameraPosition - pixelPosition);
	float3 r = reflect(lightDirection, normal);
	float specExponent = materialSpecular * MAX_SPECULAR_EXPONENT;

	float3 specularTerm;
	if (specExponent >= 0.05f)
		specularTerm = pow(saturate(dot(r, v)), specExponent);
	else
		specularTerm = float3(0, 0, 0);

	return specularTerm * pixelSpecular;
}

// Trowbridge-Reitz (GGX) Specular Distribution function
// D(h, n) = roughness^2 / (PI * ((normal dot halfAngle)^2 * (roughness^2-1) + 1)^2)
//	- normal: Normal Direction
//	- halfAngle: Half-Angle Vector between Light and Camera Vectors
//	- roughness: Roughness Value
float SpecularDistribution(float3 normal, float3 halfAngle, float roughness) 
{
	float NdotH = saturate(dot(normal, halfAngle)); // clamped to [0,1] - negative values should be black
	NdotH *= NdotH; // Square
	float a = roughness * roughness; // Remap roughness to roughness^2, as per Unreal - This must be done EVERYWHERE roughness is used
	a = max(a * a, MIN_ROUGHNESS);; // Clamp to a Min, because otherwise an infinitely small point would have infinitely small spread
	float denom = NdotH * (a - 1) + 1; // Roughness never equalling 0 stops this from ever going to 0
	denom *= denom; // Square
	return a / (PI * denom); // roughness^2 / (PI * (NdotH^2 * (roughness^2-1) + 1)^2)
}

// Fresnel Term - Schlick Approzimation (faster, almost as accurate)
// F(viewVector, halfAngle, f0) = f0 + (1 - f0)(1 - (viewVector dot halfAngle)) ^ 5
//	- normal: Normal Direction
//	- halfAngle: Half-Angle vector between Light and Camera Vectors
//	- f0: Full Specular Value when normal = 1
float3 Fresnel(float3 viewVector, float3 halfAngle, float3 f0)
{
	float VdotH = saturate(dot(viewVector, halfAngle)); // negative values are 0
	return f0 + ((1 - f0) * pow(1 - VdotH, 5)); // f0 + (1-f0)(1-VdotH)^5
}

// Geometric Shadowing - Schlick-GGX (based on Schlick-Beckmann)
// k is remapped to a / 2, roughness remapped to (r+1)/2
// This is done twice, once for the Light Vector and once for the Camera Vector
//	- n: Normal Direction
//	- f: Vector to dot with n for this iteration of the Shadowing
//	- roughness: roughness Value
float GeometricShadowing(float3 n, float3 f, float roughness)
{
	float k = pow(roughness + 1, 2) / 8.0f; // roughness remapped to (r+1)/2, then squared as per Unreal
	float NdotF = saturate(dot(n, f)); // negative side is fully shadowed
	return NdotF / (NdotF * (1 - k) + k);
}

// Cook-Torrance Microfacet Specular Light Calculation
//	- normal: normalized normal direction of pixel
//	- lightVector: normalized direction from pixel to light
//	- viewVector: normalized direction from pixel to camera/eyepoint
//	- roughness: pixel roughness value
//	- specularColor: specularColor for pixel, which is solid greyscale for non-metals and albedo for metals
float3 MicrofacetBRDF(float3 normal, float3 lightVector, float3 viewVector, float roughness, float3 specularColor) 
{
	// Half-Angle between Light Vector and View Vector. Must be halfway between them, and normalized, and normalizing
	// removes the need to divide after adding - normalize((l+v)/2) is equal to normalize(l+v)
	float3 halfAngle = normalize(lightVector + viewVector);

	// Components
	float D = SpecularDistribution(normal, halfAngle, roughness);
	float3 F = Fresnel(viewVector, halfAngle, specularColor);
	float G = GeometricShadowing(normal, viewVector, roughness) * GeometricShadowing(normal, lightVector, roughness);

	// Both dot products can be 0, which is bad, so take the max
	return (D * F * G) / (4 * max(dot(normal, viewVector), dot(normal, lightVector)));
}

// Attenutation function for ranged lights. Code taken from A7 - Lights
float Attenuate(Light light, float3 worldPos)
{
	float dist = distance(light.Position, worldPos);
	float att = saturate(1.0f - (dist * dist / (light.Range * light.Range)));
	return att * att;
}

// Conserve energy by conserving Diffuse Light
//	- diffuseLight: Total Diffuse Light for the pixel
//	- specularLight: Total Specular Light for the pixel
//	- metalness: Metalness Value for this pixel
float3 ConserveDiffuseEnergy(float3 diffuseLight, float3 specularLight, float metalness)
{
	return diffuseLight * ((1 - saturate(specularLight)) * (1 - metalness));
}

// Calculate PBR Diffuse and Specular Terms for a single Directional Light
//	- directionalLight: directional Light to calculate lighting for
//	- pixelData: all pixel data for this pixel, easier than passing individual data
//	- cameraPosition: world position of the euepoint
//	- roughness: rougness of the object - inverse shininess
//	- pixelSpecular: per-pixel specular value from a Specular Map (1 to ignore)
float3 CalculateDirectionalLightDiffuseAndSpecular(Light directionalLight, VertexToPixel pixelData, 
	float3 cameraVector, float roughness, float metalness, float3 pixelSpecular, float3 pixelColor)
{
	// Accumulate Values
	float3 lightDirection = normalize(-directionalLight.Direction);
	float3 lightColor = directionalLight.Color * directionalLight.Intensity;

	// Calculate Diffuse Light
	float3 diffuseTerm = DiffuseBRDF(lightDirection, pixelData.normal);

	// Calculate Specular Light
	float3 specularTerm = MicrofacetBRDF(pixelData.normal, lightDirection, cameraVector, roughness, pixelSpecular);
	diffuseTerm = ConserveDiffuseEnergy(diffuseTerm, specularTerm, metalness);

	return (diffuseTerm * pixelColor + specularTerm) * lightColor;
}

// Calculate PBR Diffuse and Specular Terms for a single Directional Light
//	- pointLight: point Light to calculate lighting for
//	- pixelData: all pixel data for this pixel, easier than passing individual data
//	- cameraPosition: world position of the euepoint
//	- roughness: rougness of the object - inverse shininess
//	- pixelSpecular: per-pixel specular value from a Specular Map (1 to ignore)
float3 CalculatePointLightDiffuseAndSpecular(Light pointLight, VertexToPixel pixelData,
	float3 cameraVector, float roughness, float metalness, float3 pixelSpecular, float3 pixelColor)
{
	// Gather Values
	float3 lightDirection = normalize(pointLight.Position - pixelData.worldPosition);
	float3 lightColor = pointLight.Color * pointLight.Intensity;

	// Calculate Diffuse Light
	float3 diffuseTerm = DiffuseBRDF(lightDirection, pixelData.normal);

	// Calculate Specular Light
	float3 specularTerm = MicrofacetBRDF(pixelData.normal, lightDirection, cameraVector, roughness, pixelSpecular);
	diffuseTerm = ConserveDiffuseEnergy(diffuseTerm, specularTerm, metalness);

	return (diffuseTerm * pixelColor + specularTerm) * lightColor * Attenuate(pointLight, pixelData.worldPosition);
}

// ----------------------------- PBR IBL ----------------------------------------------

// Treat a UV coordinate as a position on a plane one unit away from the origin, direction
// determined by face. Get the normalized direction to that point on that plane
//	- uv - [0,1] uv coordinate determined by the rasterizer
//	- face - face index of the plane (active face of a cubemap)
//		- 0 = +X, 1 = -X, 2 = +Y, 3 = -Y, 4 = +Z, 5 = -Z
float3 CubeDirectionFromUV(float2 uv, int face)
{
	uv = uv * 2.f - 1.f; // unpack [0,1] to [-1,1]
	float3 direction;

	// Switch on the current Cube Face to determine the components
	// of the directional Vector (ex: +Z is Front, so x is x and y
	// is inverse y and z is 1)
	switch (face) { // TODO: Replace Case magic numbers with #defines
	case 0:
		direction = normalize(float3(1.f, -uv.y, -uv.x));
		break;
	case 1:
		direction = normalize(float3(-1.f, -uv.y, uv.x));
		break;
	case 2:
		direction = normalize(float3(uv.x, 1.f, uv.y));
		break;
	case 3:
		direction = normalize(float3(uv.x, -1.f, -uv.y));
		break;
	case 4:
		direction = normalize(float3(uv.x, -uv.y, 1.f));
		break;
	case 5:
		direction = normalize(float3(-uv.x, -uv.y, -1.f));
		break;
	default:
		direction = float3(1.f, 1.f, 1.f);
		break;
	}

	return direction;
}

// Approximates the radiance integral using Importance Sampling
// https://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf
//
// Calculates a 3d direction adjusted from a given input direction (N). The adjustment is based
// off of a roughness value (roughness) and a point on a 2D grid (Xi). The 2D grid point provides
// an offset from the N direction, and the roughness value determines how blurry the reflection is
float3 ImportanceSampleGGX(float2 Xi, float roughness, float3 N)
{
	float a = roughness * roughness;

	float phi = 2 * PI * Xi.x;
	float cosTheta = sqrt((1 - Xi.y) / (1 + (a * a - 1) * Xi.y));
	float sinTheta = sqrt(1 - cosTheta * cosTheta);

	float3 H;
	H.x = sinTheta * cos(phi);
	H.y = sinTheta * sin(phi);
	H.z = cosTheta;

	float3 upVector = abs(N.z) < 0.999 ? float3(0, 0, 1) : float3(1, 0, 0);
	float3 tangentX = normalize(cross(upVector, N));
	float3 tangentY = cross(N, tangentX);
	
	// Tangent to world space
	return tangentX * H.x + tangentY * H.y + N * H.z;
}

#endif