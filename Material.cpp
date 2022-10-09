#include "Material.h"

// ----------------------------------------------------------
// Construct a basic Material with no tint
// ----------------------------------------------------------
Material::Material(std::shared_ptr<SimpleVertexShader> a_vertexShader, std::shared_ptr<SimplePixelShader> a_pixelShader)
	: Material(a_vertexShader, a_pixelShader, Color(1.f, 1.f, 1.f, 1.f))
{
}

// ----------------------------------------------------------
// Construct a basic Material with a custom Tint
// ----------------------------------------------------------
Material::Material(std::shared_ptr<SimpleVertexShader> a_vertexShader, std::shared_ptr<SimplePixelShader> a_pixelShader, Color a_colorTint)
	: m_vertexShader(a_vertexShader)
	, m_pixelShader(a_pixelShader)
	, m_colorTint(a_colorTint)
{
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
