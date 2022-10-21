#pragma once

#include "simpleshader/SimpleShader.h"
#include "Types.h"

#include <memory>

class Material
{
public:
	// Construction / Destruction
	Material(std::shared_ptr<SimpleVertexShader> a_vertexShader, std::shared_ptr<SimplePixelShader> a_pixelShader);
	Material(std::shared_ptr<SimpleVertexShader> a_vertexShader, std::shared_ptr<SimplePixelShader> a_pixelShader, Color a_colorTint, float a_roughness);
	~Material();

	// Setters
	void SetColorTint(Color a_colorTint);
	void SetRoughness(float a_roughness);
	void SetVertexShader(std::shared_ptr<SimpleVertexShader> a_vertexShader);
	void SetPixelShader(std::shared_ptr<SimplePixelShader> a_pixelShader);

	// Getters
	Color GetColorTint();
	float GetRoughness();
	std::shared_ptr<SimpleVertexShader> GetVertexShader();
	std::shared_ptr<SimplePixelShader> GetPixelShader();

protected:
	Color m_colorTint;
	float m_roughness;
	std::shared_ptr<SimpleVertexShader> m_vertexShader;
	std::shared_ptr<SimplePixelShader> m_pixelShader;
};

