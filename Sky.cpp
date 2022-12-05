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
// Creates a texture cube resource and dispatches Draw Calls
// to generate an Irradiance Map from the Sky Cube
//-------------------------------------------------------
void Sky::CreateEnvironmentMap(Microsoft::WRL::ComPtr<ID3D11Device> a_device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> a_context, std::shared_ptr<SimpleVertexShader> a_irradianceVS, std::shared_ptr<SimplePixelShader> a_irradiancePS)
{
	// Create Texture Cube Resources. Copy directly from Sky Cube (since it is an irradiance map for that)
	D3D11_SHADER_RESOURCE_VIEW_DESC skyCubeDesc = {};
	m_cubeMap->GetDesc(&skyCubeDesc);

	Microsoft::WRL::ComPtr<ID3D11Resource> resource = {};
	D3D11_TEXTURE2D_DESC skyTextureDesc;
	m_cubeMap->GetResource(resource.GetAddressOf());
	Microsoft::WRL::ComPtr<ID3D11Texture2D> texCube = static_cast<ID3D11Texture2D*>(resource.Get()); // Actual Sky TexCube
	texCube->GetDesc(&skyTextureDesc);

	// Create the Description of a face Texture
	D3D11_TEXTURE2D_DESC faceTextureDesc = {};
	faceTextureDesc.Format = skyTextureDesc.Format;
	faceTextureDesc.Width = skyTextureDesc.Width;
	faceTextureDesc.Height = skyTextureDesc.Height;
	faceTextureDesc.ArraySize = 1; // A Face is not an array
	faceTextureDesc.MipLevels = 1; // A Face only needs 1 Mip Level
	faceTextureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET; // Resource to be used in a Shader AND Render Target
	faceTextureDesc.SampleDesc.Count = 1;
	faceTextureDesc.SampleDesc.Quality = 0;

	// Create Irradiance Cube
	a_device->CreateTexture2D(&skyTextureDesc, nullptr, texCube.GetAddressOf()); // texCube can be reused

	// Save back buffer Render Target and Viewport so that it can be replaced after Textures are drawn
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> backBufferRTV;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthBufferDSV;
	a_context->OMGetRenderTargets(1, backBufferRTV.GetAddressOf(), depthBufferDSV.GetAddressOf());

	// Create the Texture Cube
	for (int i = 0; i < 6; i++) {
		// Create a Texture
		Microsoft::WRL::ComPtr<ID3D11Texture2D> faceTexture;
		a_device->CreateTexture2D(&faceTextureDesc, nullptr, faceTexture.GetAddressOf());

		// Dispatch a Draw Call with that Texture as the Render Target
		// Turn off Vertex and Input Buffers
		a_context->IASetIndexBuffer(nullptr, DXGI_FORMAT_R32_UINT, 0);
		UINT stride = sizeof(Vertex); // Stride of everything else being drawn?
		UINT offset = 0;
		ID3D11Buffer* nullBuffer = nullptr; // Must pass address of nullptr to clear, not nullptr itself
		a_context->IASetVertexBuffers(0, 1, &nullBuffer, &stride, &offset);
		
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> faceRTV;
		a_device->CreateRenderTargetView(faceTexture.Get(), nullptr, faceRTV.GetAddressOf());
		a_context->OMSetRenderTargets(1, faceRTV.GetAddressOf(), nullptr);

		float phiStep = 0.025f, thetaStep = 0.025f;
		a_irradianceVS->SetShader();
		a_irradiancePS->SetShader();
		a_irradiancePS->SetShaderResourceView("EnvMap", m_cubeMap); // Environment is this Sky
		a_irradiancePS->SetSamplerState("Sampler", m_samplerState); // Use same Sampler
		a_irradiancePS->SetInt("c_face", i); // This face
		a_irradiancePS->SetFloat("c_phiStep", phiStep);
		a_irradiancePS->SetFloat("c_thetaStep", thetaStep);
		a_irradiancePS->CopyAllBufferData();

		a_context->Draw(3, 0); // Draw with 3 vertices (full-screen triangle)
		 
		// Save that Texture to the Cubemap subresource
	}

	// Reset Render Target
	a_context->OMSetRenderTargets(1, backBufferRTV.GetAddressOf(), depthBufferDSV.Get());
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

//-------------------------------------------------------
// Retrieves the Irradiance Map of the Sky
//-------------------------------------------------------
Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Sky::GetEnvironmentMap()
{
	return m_envMap;
}
