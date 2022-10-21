#include "Material.h"

// ----------------------------------------------------------
// Construct a basic Material with no tint
// ----------------------------------------------------------
Material::Material(std::shared_ptr<SimpleVertexShader> a_vertexShader, std::shared_ptr<SimplePixelShader> a_pixelShader)
	: Material(a_vertexShader, a_pixelShader, Color(1.f, 1.f, 1.f, 1.f), 0.f)
{
}

// ----------------------------------------------------------
// Construct a basic Material with a custom Tint
// ----------------------------------------------------------
Material::Material(std::shared_ptr<SimpleVertexShader> a_vertexShader, std::shared_ptr<SimplePixelShader> a_pixelShader, Color a_colorTint, float a_roughness)
	: m_vertexShader(a_vertexShader)
	, m_pixelShader(a_pixelShader)
	, m_colorTint(a_colorTint)
	, m_roughness(a_roughness)
{
	// Bound roughness
	if (m_roughness > 1.f) m_roughness = 1.f;
	if (m_roughness < 0.f) m_roughness = 0.f;
}

// ----------------------------------------------------------
// Deconstruct and delete any heap variables
// ----------------------------------------------------------
Material::~Material()
{
	// Unused at this time - no heap variables
}

// ----------------------------------------------------------
// Set Material's universal color tint
// ----------------------------------------------------------
void Material::SetColorTint(Color a_colorTint)
{
	m_colorTint = a_colorTint;
}

// ----------------------------------------------------------
// Set Material's roughness constant (inverse shininess)
// ----------------------------------------------------------
void Material::SetRoughness(float a_roughness)
{
	m_roughness = a_roughness;

	// Bound roughness
	if (m_roughness > 1.f) m_roughness = 1.f;
	if (m_roughness < 0.f) m_roughness = 0.f;
}

// ----------------------------------------------------------
// Sets the Vertex Shader used by this Material
// ----------------------------------------------------------
void Material::SetVertexShader(std::shared_ptr<SimpleVertexShader> a_vertexShader)
{
	m_vertexShader = a_vertexShader;
}

// ----------------------------------------------------------
// Sets the Pixel Shader used by this Material
// ----------------------------------------------------------
void Material::SetPixelShader(std::shared_ptr<SimplePixelShader> a_pixelShader)
{
	m_pixelShader = a_pixelShader;
}

// ----------------------------------------------------------
// Gets the universal color tint for this Material
// ----------------------------------------------------------
Color Material::GetColorTint()
{
	return m_colorTint;
}

// ----------------------------------------------------------
// Set Material's roughness (inverse shininess)
// ----------------------------------------------------------
float Material::GetRoughness()
{
	return m_roughness;
}

// ----------------------------------------------------------
// Get a reference the Vertex Shader used by this Material
// ----------------------------------------------------------
std::shared_ptr<SimpleVertexShader> Material::GetVertexShader()
{
	return m_vertexShader;
}

// ----------------------------------------------------------
// Get a reference to the Pixel Shader used by this Material
// ----------------------------------------------------------
std::shared_ptr<SimplePixelShader> Material::GetPixelShader()
{
	return m_pixelShader;
}
