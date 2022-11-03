#include "Sky.h"

//-------------------------------------------------------
// Construct a Sky with passed in assets. Sky is not
// responsible for managing individual Textures or Shaders
//-------------------------------------------------------
Sky::Sky(Microsoft::WRL::ComPtr<ID3D11Device> a_device, std::shared_ptr<Mesh> a_mesh, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> a_cubemap, Microsoft::WRL::ComPtr<ID3D11SamplerState> a_sampler, std::shared_ptr<SimpleVertexShader> a_vertShader, std::shared_ptr<SimplePixelShader> a_pixelShader)
	: m_samplerState(a_sampler)
	, m_cubeMap(a_cubemap)
	, m_skyMesh(a_mesh)
	, m_vertexShader(a_vertShader)
	, m_pixelShader(a_pixelShader)
{
	// Create Rasterizer Description to keep only Triangles facing inward
	D3D11_RASTERIZER_DESC rasterizerDesc = {};
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
	rasterizerDesc.CullMode = D3D11_CULL_FRONT;
	a_device->CreateRasterizerState(&rasterizerDesc, m_rasterizerState.GetAddressOf());

	// Create a Depth Stencil State to allow correct representation of Skybox at depth 1.0
	D3D11_DEPTH_STENCIL_DESC depthDesc = {};
	depthDesc.DepthEnable = true;
	depthDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL; // If depth is equal to max depth, keep it
	a_device->CreateDepthStencilState(&depthDesc, m_depthState.GetAddressOf());
}

//-------------------------------------------------------
// Clean's the Sky's heap references
//-------------------------------------------------------
Sky::~Sky()
{
	// No heap data
}

//-------------------------------------------------------
// Draw the Skybox
//-------------------------------------------------------
void Sky::Draw(Microsoft::WRL::ComPtr<ID3D11DeviceContext> a_d3dContext, std::shared_ptr<Camera> a_mainCamera)
{
	// Set states
	a_d3dContext->RSSetState(m_rasterizerState.Get());
	a_d3dContext->OMSetDepthStencilState(m_depthState.Get(), 0);

	// Set Shader variables
	m_vertexShader->SetShader();
	m_pixelShader->SetShader();

	if (m_vertexShader->HasVariable("c_viewMatrix"))
		m_vertexShader->SetMatrix4x4("c_viewMatrix", a_mainCamera->GetViewMatrix());
	if (m_vertexShader->HasVariable("c_projectionMatrix"))
		m_vertexShader->SetMatrix4x4("c_projectionMatrix", a_mainCamera->GetProjectionMatrix());

	if (m_pixelShader->HasSamplerState("SkySampler"))
		m_pixelShader->SetSamplerState("SkySampler", m_samplerState.Get());
	if (m_pixelShader->HasShaderResourceView("CubeMap"))
		m_pixelShader->SetShaderResourceView("CubeMap", m_cubeMap.Get());

	m_vertexShader->CopyAllBufferData();
	m_pixelShader->CopyAllBufferData();

	// Draw Sky
	m_skyMesh->Draw();

	// Reset render states to defaults
	a_d3dContext->RSSetState(nullptr);
	a_d3dContext->OMSetDepthStencilState(nullptr, 0);
}

//-------------------------------------------------------
// Sets the Texture Sampler State used by the Sky
//-------------------------------------------------------
void Sky::SetSamplerState(Microsoft::WRL::ComPtr<ID3D11SamplerState> a_samplerState)
{
	m_samplerState = a_samplerState;
}

//-------------------------------------------------------
// Sets the Texture Cube used by the Sky
//-------------------------------------------------------
void Sky::SetCubeMap(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> a_cubeMap)
{
	m_cubeMap = a_cubeMap;
}

//-------------------------------------------------------
// Sets the Depth Stencil State used by the Sky
//-------------------------------------------------------
void Sky::SetDepthStencilState(Microsoft::WRL::ComPtr<ID3D11DepthStencilState> a_depthState)
{
	m_depthState = a_depthState;
}

//-------------------------------------------------------
// Sets the Rasterizer State used by the Sky
//-------------------------------------------------------
void Sky::SetRasterizerState(Microsoft::WRL::ComPtr<ID3D11RasterizerState> a_rasterizerState)
{
	m_rasterizerState = a_rasterizerState;
}

//-------------------------------------------------------
// Sets the actual Mesh used by the Sky
//-------------------------------------------------------
void Sky::SetSkyBox(std::shared_ptr<Mesh> a_mesh)
{
	m_skyMesh = a_mesh;
}

//-------------------------------------------------------
// Sets the Vertex Shader used by the Sky
//-------------------------------------------------------
void Sky::SetVertexShader(std::shared_ptr<SimpleVertexShader> a_vertexShader)
{
	m_vertexShader = a_vertexShader;
}

//-------------------------------------------------------
// Sets the Pixel Shader used by the Sky
//-------------------------------------------------------
void Sky::SetPixelShader(std::shared_ptr<SimplePixelShader> a_pixelShader)
{
	m_pixelShader = a_pixelShader;
}

//-------------------------------------------------------
// Retrieves the Texture Sampler State used by the Sky
//-------------------------------------------------------
Microsoft::WRL::ComPtr<ID3D11SamplerState> Sky::GetSamplerState()
{
	return m_samplerState;
}

//-------------------------------------------------------
// Retrieves the Texture Cube used by the Sky
//-------------------------------------------------------
Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Sky::GetCubeMap()
{
	return m_cubeMap;
}

//-------------------------------------------------------
// Retrieves the Depth Stencil State used by the Sky
//-------------------------------------------------------
Microsoft::WRL::ComPtr<ID3D11DepthStencilState> Sky::GetDepthStencilState()
{
	return m_depthState;
}

//-------------------------------------------------------
// Retrieves the Rasterizer State used by the Sky
//-------------------------------------------------------
Microsoft::WRL::ComPtr<ID3D11RasterizerState> Sky::GetRasterizerState()
{
	return m_rasterizerState;
}

//-------------------------------------------------------
// Retrieves the actual Mesh used by the Sky
//-------------------------------------------------------
std::shared_ptr<Mesh> Sky::GetMesh()
{
	return m_skyMesh;
}

//-------------------------------------------------------
// Retrieves the Vertex Shader used by the Sky
//-------------------------------------------------------
std::shared_ptr<SimpleVertexShader> Sky::GetVertexShader()
{
	return m_vertexShader;
}

//-------------------------------------------------------
// Retrieves the Pixel Shader used by the Sky
//-------------------------------------------------------
std::shared_ptr<SimplePixelShader> Sky::GetPixelShader()
{
	return m_pixelShader;
}
