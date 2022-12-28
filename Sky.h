#pragma once

#include <wrl/client.h> // Used for ComPtr - a smart pointer for COM objects
#include <memory>

#include "Mesh.h"
#include "Camera.h"
#include "simpleshader/SimpleShader.h"

//-------------------------------------------------------
// A Sky represents the unmoving backdrop of the scene,
// rendered using its own shaders
//-------------------------------------------------------
class Sky
{
public:
	Sky(Microsoft::WRL::ComPtr<ID3D11Device> a_device, std::shared_ptr<Mesh> a_mesh, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> a_cubemap,
		Microsoft::WRL::ComPtr<ID3D11SamplerState> a_sampler, std::shared_ptr<SimpleVertexShader> a_vertShader, std::shared_ptr<SimplePixelShader> a_pixelShader);
	~Sky();

	void Draw(Microsoft::WRL::ComPtr<ID3D11DeviceContext> a_d3dContext, std::shared_ptr<Camera> a_mainCamera);

	void CreateEnvironmentMap(Microsoft::WRL::ComPtr<ID3D11Device> a_device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> a_context, std::shared_ptr<SimpleVertexShader> a_irradianceVS, std::shared_ptr<SimplePixelShader> a_irradiancePS);
	void CreateSpecularReflectanceMap(Microsoft::WRL::ComPtr<ID3D11Device> a_device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> a_context, std::shared_ptr<SimpleVertexShader> a_vertShader, std::shared_ptr<SimplePixelShader> a_prefilterPS);

	// Setters
	void SetSamplerState(Microsoft::WRL::ComPtr<ID3D11SamplerState> a_samplerState);
	void SetCubeMap(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> a_cubeMap);
	void SetDepthStencilState(Microsoft::WRL::ComPtr<ID3D11DepthStencilState> a_depthState);
	void SetRasterizerState(Microsoft::WRL::ComPtr<ID3D11RasterizerState> a_rasterizerState);
	void SetSkyBox(std::shared_ptr<Mesh> a_mesh);
	void SetVertexShader(std::shared_ptr<SimpleVertexShader> a_vertexShader);
	void SetPixelShader(std::shared_ptr<SimplePixelShader> a_pixelShader);

	// Getters
	Microsoft::WRL::ComPtr<ID3D11SamplerState> GetSamplerState();
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetCubeMap();
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> GetDepthStencilState();
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> GetRasterizerState();
	std::shared_ptr<Mesh> GetMesh();
	std::shared_ptr<SimpleVertexShader> GetVertexShader();
	std::shared_ptr<SimplePixelShader> GetPixelShader();

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetEnvironmentMap();
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetReflectanceMap();

protected:
	D3D11_TEXTURE2D_DESC GetTextureCubeDescription();
	D3D11_SHADER_RESOURCE_VIEW_DESC GetCubeSRVDescription();

	Microsoft::WRL::ComPtr<ID3D11SamplerState> m_samplerState; // Texture Sampler
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_cubeMap; // Texture Cube
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> m_depthState; // Depth Buffer State
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_rasterizerState; // Rasterizer must cull opposite face
	std::shared_ptr<Mesh> m_skyMesh; // Actual mesh to render as the sky
	std::shared_ptr<SimpleVertexShader> m_vertexShader; // Sky-specific shader
	std::shared_ptr<SimplePixelShader> m_pixelShader; // Sky-specific shader

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_envMap; // Texture Cube holding the Sky's irradiance map
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_specMap; // Texture Cube holding the Sky's prefiltered reflectance map
};

