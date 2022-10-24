#pragma once

#include "simpleshader/SimpleShader.h"
#include "Types.h"

#include <memory>
#include <unordered_map>
#include <wrl/client.h>
#include <d3d11.h>

class Material
{
public:
	// Construction / Destruction
	Material(std::shared_ptr<SimpleVertexShader> a_vertexShader, std::shared_ptr<SimplePixelShader> a_pixelShader);
	Material(std::shared_ptr<SimpleVertexShader> a_vertexShader, std::shared_ptr<SimplePixelShader> a_pixelShader, Color a_colorTint, float a_roughness);
	~Material();

	// General Operations
	void PrepareMaterial();

	// Setters
	void SetColorTint(Color a_colorTint);
	void SetRoughness(float a_roughness);
	void SetUVOffset(Vector2 a_offset);
	void SetUVScale(float a_scale);
	void SetVertexShader(std::shared_ptr<SimpleVertexShader> a_vertexShader);
	void SetPixelShader(std::shared_ptr<SimplePixelShader> a_pixelShader);

	// Getters
	Color GetColorTint();
	float GetRoughness();
	Vector2 GetUVOffset();
	float GetUVScale();
	std::shared_ptr<SimpleVertexShader> GetVertexShader();
	std::shared_ptr<SimplePixelShader> GetPixelShader();

	// Textures
	void AddTextureSRV(std::string a_name, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> a_srv);
	void AddSampler(std::string a_name, Microsoft::WRL::ComPtr<ID3D11SamplerState> a_sampler);

protected:
	Color m_colorTint;
	float m_roughness;

	Vector2 m_uvOffset;
	float m_uvScale;

	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> m_textureSRVs;
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D11SamplerState>> m_samplers;

	std::shared_ptr<SimpleVertexShader> m_vertexShader;
	std::shared_ptr<SimplePixelShader> m_pixelShader;
};

