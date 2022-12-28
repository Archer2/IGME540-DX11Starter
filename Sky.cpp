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
	D3D11_SHADER_RESOURCE_VIEW_DESC skyCubeDesc = GetCubeSRVDescription();
	D3D11_TEXTURE2D_DESC skyTextureDesc = GetTextureCubeDescription();

	// Adjust Texture Settings
	skyTextureDesc.Width /= 16;  // Width and height can be massively reduced, because there is very little detail in the 
	skyTextureDesc.Height /= 16; // irradiance map. Assuming a minimum Skybox dimension of 1024x1024, the result will be 64x64
	skyTextureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;

	// Create Irradiance Cube
	Microsoft::WRL::ComPtr<ID3D11Texture2D> irrMap;
	a_device->CreateTexture2D(&skyTextureDesc, nullptr, irrMap.GetAddressOf());

	// Save back buffer Render Target and Viewport so that it can be replaced after Textures are drawn
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> backBufferRTV;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthBufferDSV;
	a_context->OMGetRenderTargets(1, backBufferRTV.GetAddressOf(), depthBufferDSV.GetAddressOf());
	UINT viewportCount = 1;
	D3D11_VIEWPORT cachedViewport;
	a_context->RSGetViewports(&viewportCount, &cachedViewport);
	
	// Reset viewPort to skyTexture dimensions
	D3D11_VIEWPORT textureViewport = {};
	textureViewport.MaxDepth = 1.f;
	textureViewport.Width = (FLOAT)skyTextureDesc.Width;
	textureViewport.Height = (FLOAT)skyTextureDesc.Height;
	a_context->RSSetViewports(1, &textureViewport);

	// Turn off Vertex and Input Buffers
	a_context->IASetIndexBuffer(nullptr, DXGI_FORMAT_R32_UINT, 0);
	UINT stride = sizeof(Vertex); // Stride of everything else being drawn?
	UINT offset = 0;
	ID3D11Buffer* nullBuffer = nullptr; // Must pass address of nullptr to clear, not nullptr itself
	a_context->IASetVertexBuffers(0, 1, &nullBuffer, &stride, &offset);

	// Create the Texture Cube
	for (int i = 0; i < 6; i++) {
		D3D11_RENDER_TARGET_VIEW_DESC viewDesc = {};
		viewDesc.Format = skyTextureDesc.Format;
		viewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
		viewDesc.Texture2DArray.MipSlice = 0;
		viewDesc.Texture2DArray.ArraySize = 1;
		viewDesc.Texture2DArray.FirstArraySlice = i;

		// Dispatch a Draw Call with that Texture as the Render Target		
		// Render Target for texture face
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> faceRTV;
		a_device->CreateRenderTargetView(irrMap.Get(), &viewDesc, faceRTV.GetAddressOf());
		a_context->OMSetRenderTargets(1, faceRTV.GetAddressOf(), nullptr);

		// Set shaders and data
		float phiStep = 0.05f /*0.025f*/, thetaStep = 0.05f /*0.025f*/; // Adjust for hardware, or alternatively adjust dimensions
		a_irradianceVS->SetShader();
		a_irradiancePS->SetShader();
		a_irradiancePS->SetShaderResourceView("EnvMap", m_cubeMap); // Environment is this Sky
		a_irradiancePS->SetSamplerState("Sampler", m_samplerState); // Use same Sampler
		a_irradiancePS->SetInt("c_face", i); // This face
		a_irradiancePS->SetFloat("c_phiStep", phiStep);
		a_irradiancePS->SetFloat("c_thetaStep", thetaStep);
		a_irradiancePS->CopyAllBufferData();

		a_context->Draw(3, 0); // Draw with 3 vertices (full-screen triangle)
	}

	// Create the SRV for the Irradiance Map
	a_device->CreateShaderResourceView(irrMap.Get(), &skyCubeDesc, m_envMap.GetAddressOf());

	// Reset Render Target and Viewport
	a_context->OMSetRenderTargets(1, backBufferRTV.GetAddressOf(), depthBufferDSV.Get());
	a_context->RSSetViewports(1, &cachedViewport);
}

//-------------------------------------------------------
// Creates a texture cube resource and dispatches Draw Calls
// to prefilter the Sky into a reflectance map for multiple
// roughness levels (0-1 in .1 increments);
//-------------------------------------------------------
void Sky::CreateSpecularReflectanceMap(Microsoft::WRL::ComPtr<ID3D11Device> a_device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> a_context, std::shared_ptr<SimpleVertexShader> a_vertShader, std::shared_ptr<SimplePixelShader> a_prefilterPS)
{
	// Copy SRV Description from the sky, since the reflectance map is based off of that
	D3D11_SHADER_RESOURCE_VIEW_DESC skyCubeDesc = GetCubeSRVDescription();
	D3D11_TEXTURE2D_DESC skyTextureDesc = GetTextureCubeDescription();

	// Adjust texture settings. Must add Mip Levels
	int ignoredSmallMips = 2; // ignore mips 2x2 and 4x4 (1x1 is so useless as to not even be relevant) 
	skyTextureDesc.Width /= 8;  // Width and height can be massively reduced, because there is very little detail in the 
	skyTextureDesc.Height /= 8; // irradiance map. Assuming a minimum Skybox dimension of 1024x1024, the result will be 128x128
	skyTextureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	// MipLevels is the power of 2 of the size (this ignores 0-indexing, which is just the 1x1 level, which is unnecessary)
	skyTextureDesc.MipLevels = max((int)(log2(skyTextureDesc.Width) - ignoredSmallMips), 1); // Subtract ignored Mip levels from the total count

	// Adjust SRV for the new CubeMap to account for multiple Mips
	skyCubeDesc.TextureCube.MipLevels = skyTextureDesc.MipLevels; // Match the Texture Mips
	skyCubeDesc.TextureCube.MostDetailedMip = 0; // Highest mip = most detailed

	// Create Reflection Map
	Microsoft::WRL::ComPtr<ID3D11Texture2D> refMap;
	a_device->CreateTexture2D(&skyTextureDesc, nullptr, refMap.GetAddressOf());

	// Create viewport for rasterizing
	D3D11_VIEWPORT textureViewport = {};
	textureViewport.MaxDepth = 1.f;
	textureViewport.Width = skyTextureDesc.Width * 2.f; // This will be divided in the loop to be the correct 1st size
	textureViewport.Height = textureViewport.Width; // This will be divided in the loop to be the correct 1st size

	// Save back buffer Render Target and Viewport so that it can be replaced after Textures are drawn
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> backBufferRTV;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthBufferDSV;
	a_context->OMGetRenderTargets(1, backBufferRTV.GetAddressOf(), depthBufferDSV.GetAddressOf());
	UINT viewportCount = 1;
	D3D11_VIEWPORT cachedViewport;
	a_context->RSGetViewports(&viewportCount, &cachedViewport);

	// Turn off Vertex and Input Buffers
	a_context->IASetIndexBuffer(nullptr, DXGI_FORMAT_R32_UINT, 0);
	UINT stride = sizeof(Vertex); // Stride of everything else being drawn?
	UINT offset = 0;
	ID3D11Buffer* nullBuffer = nullptr; // Must pass address of nullptr to clear, not nullptr itself
	a_context->IASetVertexBuffers(0, 1, &nullBuffer, &stride, &offset);

	// Set shaders and data shared by all faces and mips
	a_vertShader->SetShader();
	a_prefilterPS->SetShader();
	a_prefilterPS->SetShaderResourceView("EnvMap", m_cubeMap); // Environment is this Sky
	a_prefilterPS->SetSamplerState("Sampler", m_samplerState); // Use same Sampler

	// Loop through mip levels (.2 increments of roughness)
	for (int mip = 0; mip < skyTextureDesc.MipLevels; mip++) {
		// Reset viewPort to skyTexture dimensions at this Mip level and set it, shared between faces
		textureViewport.Width /= 2.f;
		textureViewport.Height /= 2.f;
		a_context->RSSetViewports(1, &textureViewport);

		// Create the Cube at that interval
		for (int face = 0; face < 6; face++) {
			// Create Viewport and RTV for this Face and Mip
			D3D11_RENDER_TARGET_VIEW_DESC viewDesc = {};
			viewDesc.Format = skyTextureDesc.Format;
			viewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
			viewDesc.Texture2DArray.MipSlice = mip;
			viewDesc.Texture2DArray.FirstArraySlice = face;
			viewDesc.Texture2DArray.ArraySize = 1;

			// Dispatch a Draw Call with that Texture as the Render Target		
			Microsoft::WRL::ComPtr<ID3D11RenderTargetView> faceRTV;
			a_device->CreateRenderTargetView(refMap.Get(), &viewDesc, faceRTV.GetAddressOf());
			a_context->OMSetRenderTargets(1, faceRTV.GetAddressOf(), nullptr);

			// Set shader data
			a_prefilterPS->SetInt("c_face", face); // This face
			a_prefilterPS->SetFloat("c_roughness", mip / (float)(skyTextureDesc.MipLevels - 1)); // This roughness
			a_prefilterPS->CopyAllBufferData();

			a_context->Draw(3, 0); // Draw with 3 vertices (full-screen triangle)
		}
	}

	// Create the SRV for the Reflection Map
	a_device->CreateShaderResourceView(refMap.Get(), &skyCubeDesc, m_specMap.GetAddressOf());

	// Reset Render Target and Viewport (IA Buffers are the next objects responsibility. Content was just set to null)
	a_context->OMSetRenderTargets(1, backBufferRTV.GetAddressOf(), depthBufferDSV.Get());
	a_context->RSSetViewports(1, &cachedViewport);
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

//-------------------------------------------------------
// Retrieves the Reflectance Map of the Sky for IBL Specular
//-------------------------------------------------------
Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Sky::GetReflectanceMap()
{
	return m_specMap;
}

//-------------------------------------------------------
// Retrieves the Texture Description of the Sky Texture Cube
//-------------------------------------------------------
D3D11_TEXTURE2D_DESC Sky::GetTextureCubeDescription()
{
	D3D11_TEXTURE2D_DESC texDesc; // Description of the Sky CubeMap
	Microsoft::WRL::ComPtr<ID3D11Resource> resource = {}; // Resource stored by the SRV

	m_cubeMap->GetResource(resource.GetAddressOf()); // Get the resource (texture)
	Microsoft::WRL::ComPtr<ID3D11Texture2D> texCube = static_cast<ID3D11Texture2D*>(resource.Get()); // Cast to Sky TexCube
	texCube->GetDesc(&texDesc); // Get Texture Description

	return texDesc;
}

//-------------------------------------------------------
// Retrieves the Texture Description of the Sky SRV
//-------------------------------------------------------
D3D11_SHADER_RESOURCE_VIEW_DESC Sky::GetCubeSRVDescription()
{
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	m_cubeMap->GetDesc(&srvDesc);
	return srvDesc;
}
