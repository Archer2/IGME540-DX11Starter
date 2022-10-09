#pragma once

#include "simpleshader/SimpleShader.h"
#include "Types.h"

#include <memory>

class Material
{
public:
	// Construction / Destruction
	Material(std::shared_ptr<SimpleVertexShader> a_vertexShader, std::shared_ptr<SimplePixelShader> a_pixelShader);
	Material(std::shared_ptr<SimpleVertexShader> a_vertexShader, std::shared_ptr<SimplePixelShader> a_pixelShader, Color a_colorTint);
	~Material();

	// Setters
	void SetColorTint(Color a_colorTint);
	void SetVertexShader(std::shared_ptr<SimpleVertexShader> a_vertexShader);
	void SetPixelShader(std::shared_ptr<SimplePixelShader> a_pixelShader);

	// Getters
	Color GetColorTint();
	std::shared_ptr<SimpleVertexShader> GetVertexShader();
	std::shared_ptr<SimplePixelShader> GetPixelShader();

protected:
	Color m_colorTint;
	std::shared_ptr<SimpleVertexShader> m_vertexShader;
	std::shared_ptr<SimplePixelShader> m_pixelShader;
};

