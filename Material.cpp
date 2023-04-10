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
	, m_uvOffset(0.f, 0.f)
	, m_uvScale(1.f)
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
// Set up shader data used by this Material. Ideally would
// be wholly independent of Entity, so an Entity would call
// this function and not need to pass in a whole host of its
// own values to be passed to a shader. Current architecture
// has each Entity setting values itself, with this function
// setting variables that are completely Material-dependant
// ----------------------------------------------------------
void Material::PrepareMaterial()
{
	// Set all stored texture Resource Views
	for (auto& t : m_textureSRVs) {
		const char* name = t.first.c_str();
		if (m_pixelShader->HasShaderResourceView(name)) {
			m_pixelShader->SetShaderResourceView(name, t.second);
		}
	}

	// Set all stored Sampler States
	for (auto& s : m_samplers) {
		const char* name = s.first.c_str();
		if (m_pixelShader->HasSamplerState(name)) {
			m_pixelShader->SetSamplerState(name, s.second);
		}
	}

	// Set universal UV values
	if (m_pixelShader->HasVariable("c_uvOffset"))
		m_pixelShader->SetFloat2("c_uvOffset", m_uvOffset);
	if (m_pixelShader->HasVariable("c_uvScale"))
		m_pixelShader->SetFloat("c_uvScale", m_uvScale);

	// Set miscellaneous Material values
	if (m_pixelShader->HasVariable("c_color"))
		m_pixelShader->SetFloat4("c_color", m_colorTint);
	if (m_pixelShader->HasVariable("c_roughnessScale"))
		m_pixelShader->SetFloat("c_roughnessScale", m_roughness);
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
// Sets the universal UV Offset for this Material
// ----------------------------------------------------------
void Material::SetUVOffset(Vector2 a_offset)
{
	m_uvOffset = a_offset;
}

// ----------------------------------------------------------
// Sets the universal UV Scale used by this Material
// ----------------------------------------------------------
void Material::SetUVScale(float a_scale)
{
	m_uvScale = a_scale;
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
// Gets the universal UV Offset used by this Material
// ----------------------------------------------------------
Vector2 Material::GetUVOffset()
{
	return m_uvOffset;
}

// ----------------------------------------------------------
// Gets the universal UV Scale used by this Material
// ----------------------------------------------------------
float Material::GetUVScale()
{
	return m_uvScale;
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

// ----------------------------------------------------------
// Add a Texture Resource View to the Material
// ----------------------------------------------------------
void Material::AddTextureSRV(std::string a_name, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> a_srv)
{
	m_textureSRVs.insert({ a_name, a_srv });
}

// ----------------------------------------------------------
// Add a texture Sampler State to the Material
// ----------------------------------------------------------
void Material::AddSampler(std::string a_name, Microsoft::WRL::ComPtr<ID3D11SamplerState> a_sampler)
{
	m_samplers.insert({ a_name, a_sampler });
}
