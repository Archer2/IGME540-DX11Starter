#ifndef _SHADER_COMMON_INCLUDES_
#define _SHADER_COMMON_INCLUDES_

// Macros describing type integers for BasicLight Types
// - Must match LightType enum definition in Lights.h in C++
#define LIGHT_TYPE_DIRECTIONAL 0;
#define LIGHT_TYPE_POINT 1;
#define LIGHT_TYPE_SPOT 2;

// Macro for the largest possible Specular Light exponent
#define MAX_SPECULAR_EXPONENT 256.0f;

// Struct defining basic Light properties for all basic types
// - Must match BasicLight struct definition in Lights.h
// - A good way to remove branching is probably to create different
//	 structs, one for each Light only holding that Light's values.
//	 Then pass in an array of each type of Light
struct Light {
	int Type;
	float3 Position;
	float Range;
	float3 Direction;
	float Intensity;
	float3 Color;
	float SpotAngle;
	float3 _padding;
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

// Basic Diffuse Light calculation
//	- directionToLight: direction from pixel to Light
//	- normal: normal of pixel
//	- totalLightColor: color of Light * intensity of Light
//	- surfaceColor: natural color of pixel
float4 DiffuseBRDF(float3 directionToLight, float3 normal, float4 totalLightColor, float4 surfaceColor)
{
	return saturate(dot(normal, directionToLight)) * totalLightColor * surfaceColor;
}

// Specular Light Calculation. WARNING - BRANCHING CODE - branches should all be uniform
//	- normal: normal of pixel
//	- pixelPosition: world position of the pixel
//	- cameraPosition: world position of the eyepoint
//	- lightDirection: direction of the Light - Light TO Pixel
//	- roughness: roughness constant of the material (inverse shininess)
//	- pixelSpecular: specular value of this pixel from a SpecMap (1 to ignore)
float4 CalculateSpecular(float3 normal, float3 pixelPosition, float3 cameraPosition, 
	float3 lightDirection, float roughness, float pixelSpecular)
{
	float3 v = normalize(cameraPosition - pixelPosition);
	float3 r = reflect(lightDirection, normal);
	float specExponent = (1.f - roughness) * MAX_SPECULAR_EXPONENT;

	float4 specularTerm;
	if (specExponent >= 0.05f)
		specularTerm = pow(saturate(dot(r, v)), specExponent);
	else
		specularTerm = float4(0, 0, 0, 0);

	return specularTerm * pixelSpecular;
}

// Basic Specular Light calculation
//	- specular: specular light value. Calculated with CalculateSpecular
//	- totalLightColor: color of Light * intensity of Light
//	- surfaceColor: natural color of pixel
float4 SpecularBRDF(float4 specular, float4 totalLightColor, float4 surfaceColor)
{
	return specular * totalLightColor * surfaceColor;
}

// Attenutation function for ranged lights. Code taken from A7 - Lights
float Attenuate(Light light, float3 worldPos)
{
	float dist = distance(light.Position, worldPos);
	float att = saturate(1.0f - (dist * dist / (light.Range * light.Range)));
	return att * att;
}

// Calculate Diffuse and Specular Terms for a single Directional Light
//	- directionalLight: directional Light to calculate lighting for
//	- pixelData: all pixel data for this pixel, easier than passing individual data
//	- cameraPosition: world position of the euepoint
//	- roughness: rougness of the object - inverse shininess
//	- color: TEMPORARY - universal color of the object
//	- pixelSpecular: per-pixel specular value from a Specular Map (1 to ignore)
float4 CalculateDirectionalLightDiffuseAndSpecular(Light directionalLight, VertexToPixel pixelData, 
	float3 cameraPosition, float roughness, float4 color, float pixelSpecular)
{
	// Calculate Diffuse Light
	float3 lightDirection = normalize(directionalLight.Direction);
	float4 lightColor = float4(directionalLight.Color, 1) * directionalLight.Intensity;

	float4 diffuseTerm = DiffuseBRDF(-lightDirection, pixelData.normal, lightColor, color);

	// Calculate Specular Light
	float4 spec = CalculateSpecular(pixelData.normal, pixelData.worldPosition, cameraPosition, 
		lightDirection, roughness, pixelSpecular);
	spec *= any(diffuseTerm); // Cut specular if diffuse contribution is 0
	float4 specularTerm = SpecularBRDF(spec, lightColor, color);

	return diffuseTerm + specularTerm;
}

// Calculate Diffuse and Specular Terms for a single Directional Light
//	- pointLight: point Light to calculate lighting for
//	- pixelData: all pixel data for this pixel, easier than passing individual data
//	- cameraPosition: world position of the euepoint
//	- roughness: rougness of the object - inverse shininess
//	- color: TEMPORARY - universal color of the object
//	- pixelSpecular: per-pixel specular value from a Specular Map (1 to ignore)
float4 CalculatePointLightDiffuseAndSpecular(Light pointLight, VertexToPixel pixelData,
	float3 cameraPosition, float roughness, float4 color, float pixelSpecular)
{
	// Calculate Diffuse Light
	float3 lightDirection = normalize(pixelData.worldPosition - pointLight.Position);
	float4 lightColor = float4(pointLight.Color, 1) * pointLight.Intensity;

	float4 diffuseTerm = DiffuseBRDF(-lightDirection, pixelData.normal, lightColor, color);

	// Calculate Specular Light
	float4 spec = CalculateSpecular(pixelData.normal, pixelData.worldPosition, cameraPosition, 
		lightDirection, roughness, pixelSpecular);
	spec *= any(diffuseTerm); // Cut specular if diffuse contribution is 0
	float4 specularTerm = SpecularBRDF(spec, lightColor, color);

	return (diffuseTerm + specularTerm) * Attenuate(pointLight, pixelData.worldPosition);
}

#endif