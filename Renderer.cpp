#include "Renderer.h"
#include "Helpers.h"

// Possibly not all of the imgui headers are necessary, since no setup is being done here
#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx11.h"

#include "simpleshader/SimpleShader.h"

using namespace DirectX;

//----------------------------------------------------
// Initializes objects needed for rendering. Some of 
// these can be reworked (along with larger architecture
// revamps) to be obtained elsewhere rather than stored
// individually in several places
//----------------------------------------------------
Renderer::Renderer(
	Microsoft::WRL::ComPtr<ID3D11Device> a_device,
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> a_context,
	Microsoft::WRL::ComPtr<IDXGISwapChain> a_swapChain,
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> a_backBufferRTV,
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> a_depthBufferDSV,
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> a_iblBRDFLookupTexture,
	std::shared_ptr<SimpleVertexShader> a_fullscreenVS,
	unsigned int a_windowWidth, unsigned int a_windowHeight
)
	: m_device(a_device)
	, m_context(a_context)
	, m_swapChain(a_swapChain)
	, m_backBufferRTV(a_backBufferRTV)
	, m_depthBufferDSV(a_depthBufferDSV)
	, m_iblBRDFLookupTexture(a_iblBRDFLookupTexture)
	, m_postProcessVS(a_fullscreenVS)
	, m_windowWidth(a_windowWidth)
	, m_windowHeight(a_windowHeight)
{
	// Build Resources, RTVs, and SRVs for multiple render targets
	//	- Targets needed is pretty narrowed in to the specific post-process (in this case SSAO),
	//	  which is fine for such a limited application. Generalizing this to make all RTs for a
	//	  lot of processes is iffy, but workable since only 8 RTs can be bound at once - perhaps
	//	  different Renderers build different RTs?
	//	- Just call into PostResize, since it takes care of everything anyways
	PostResize(m_windowWidth, m_windowHeight, m_backBufferRTV, m_depthBufferDSV);

	// Create SSAO-related resources
	m_ssaoCorePS = std::make_shared<SimplePixelShader>(m_device, m_context, FixPath(L"ScreenSpaceAmbientOcclusionPS.cso").c_str());
	m_ssaoBlurPS = std::make_shared<SimplePixelShader>(m_device, m_context, FixPath(L"FourByFourBlurPS.cso").c_str());
	m_ssaoCombinePS = std::make_shared<SimplePixelShader>(m_device, m_context, FixPath(L"SSAOCombinePS.cso").c_str());

	const int offsetTextureSize = 4, totalPixels = offsetTextureSize * offsetTextureSize;
	Color randomPixels[totalPixels] = {};
	for (int i = 0; i < totalPixels; i++) {
		XMVECTOR randVec = XMVectorSet(
			GenerateRandomFloat(-1.f, 1.f),
			GenerateRandomFloat(-1.f, 1.f),
			0.f, 0.f);
		XMStoreFloat4(&randomPixels[i], XMVector3Normalize(randVec));
	}
	// Create Texture and SRV
	D3D11_TEXTURE2D_DESC randDesc = {};
	randDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT; // Store raw float data (should pack in, but a 4x4 texture is fine)
	randDesc.ArraySize = 1;
	randDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	randDesc.Height = offsetTextureSize;
	randDesc.Width = offsetTextureSize;
	randDesc.MipLevels = 1;
	randDesc.Usage = D3D11_USAGE_IMMUTABLE;
	randDesc.SampleDesc.Count = 1;

	D3D11_SUBRESOURCE_DATA data = {};
	data.SysMemPitch = offsetTextureSize * sizeof(Color);
	data.pSysMem = &randomPixels[0];

	Microsoft::WRL::ComPtr<ID3D11Texture2D> tex;
	m_device->CreateTexture2D(&randDesc, &data, tex.GetAddressOf());

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = randDesc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;
	m_device->CreateShaderResourceView(tex.Get(), &srvDesc, m_ssaoRandomOffsets.GetAddressOf());

	// Generate vector of offset values
	const int offsetVectorCount = 64; // Max size of array in shader cbuffer
	Vector4 ssaoOffsets[offsetVectorCount] = {};
	for (int i = 0; i < offsetVectorCount; i++) {
		ssaoOffsets[i] = Vector4(
			GenerateRandomFloat(-1.f, 1.f),
			GenerateRandomFloat(-1.f, 1.f),
			GenerateRandomFloat(), 0.f);
		XMVECTOR offset = XMVector3Normalize(XMLoadFloat4(&ssaoOffsets[i]));

		// Scale over array size such that more values are closer to the minimum than maximum - why?
		float scale = (float)i / 64.f;
		XMVECTOR accelerateScale = XMVectorLerp(
			XMVectorSet(0.1f, 0.1f, 0.1f, 1.f),
			XMVectorSet(1.f, 1.f, 1.f, 1.f), scale * scale); // Why scale * scale?
		XMStoreFloat4(&ssaoOffsets[i], XMVectorMultiply(offset, accelerateScale));
	}
	m_ssaoOffsets = std::vector<Vector4>();
	m_ssaoOffsets.insert(m_ssaoOffsets.begin(), ssaoOffsets, ssaoOffsets + offsetVectorCount);

	// Fill out sampler states
	D3D11_SAMPLER_DESC sampleDesc = {};
	sampleDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampleDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampleDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampleDesc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT; // Because of offset vectors use Linear Interp?
	sampleDesc.MaxLOD = D3D11_FLOAT32_MAX;
	m_device->CreateSamplerState(&sampleDesc, m_standardSampler.GetAddressOf());

	sampleDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampleDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampleDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	m_device->CreateSamplerState(&sampleDesc, m_clampSampler.GetAddressOf());
}

//----------------------------------------------------
// Perfrom necessary operations that must take place
// before a screen re-size operation is performed
//	- Mainly resets the Back Buffer and Depth Buffer
//	- Requiring calls into the Renderer for this is
//	  a good pointer to architecture improvements -
//	  such a coupling should be unnecessary
//	- Clear ALL Render Targets and their SRVs
//----------------------------------------------------
void Renderer::PreResize()
{
	// Resetting Buffers here does NOT clear all references - one still exists
	// within DXCore that it must clear itself
	//	- Yet another point towards restructuring this to ask for references instead
	//	  of storing them everywhere
	m_backBufferRTV.Reset();
	m_depthBufferDSV.Reset();

	for (Microsoft::WRL::ComPtr<ID3D11RenderTargetView>& rtv : m_mrtRTVs) {
		rtv.Reset();
	}

	for (Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& srv : m_mrtSRVs) {
		srv.Reset();
	}
}

//----------------------------------------------------
// Update Renderer stored values with new information
//	- Again, this is an unnecessary step - this data
//	  should be more centralized
//	- This WILL need to update all secondary RTs and their
//	  View objects - TODO: Do this! For now just don't resize the screen...
//----------------------------------------------------
void Renderer::PostResize(unsigned int a_windowWidth, unsigned int a_windowHeight,
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> a_backBufferRTV,
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> a_depthBufferDSV)
{
	m_windowWidth = a_windowWidth;
	m_windowHeight = a_windowHeight;
	m_backBufferRTV = a_backBufferRTV;
	m_depthBufferDSV = a_depthBufferDSV;

	// Rebuild Render Target textures and their RTVs and SRVs
	D3D11_TEXTURE2D_DESC rtDesc = {};
	rtDesc.Width = m_windowWidth;
	rtDesc.Height = m_windowHeight;
	rtDesc.ArraySize = 1;
	rtDesc.MipLevels = 1;
	rtDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	rtDesc.Usage = D3D11_USAGE_DEFAULT;
	rtDesc.SampleDesc.Count = 1;

	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Texture2D.MipSlice = 0;

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;

	// Loop through all RenderTarget versions and create them
	for (int i = 0; i < RenderTarget::RT_COUNT; i++) {
		// Set texture formats. Needed here since Depth target needs to be a single higher-precision float
		//	- A more robust method is probably a helper function that takes in an RT index and Format, because
		//	  this is absolutely not scalable if multiple RTs need different storage types (like for better
		//	  packing)
		//	- Default format really should match Back Buffer exactly, but getting the actual description and format
		//	  for that is a lot when it is already assumed to be R8G8B8A8_UNORM
		rtDesc.Format = (i == RenderTarget::RT_SCENE_DEPTH) ? DXGI_FORMAT_R32_FLOAT : DXGI_FORMAT_R8G8B8A8_UNORM;
		rtvDesc.Format = rtDesc.Format;
		srvDesc.Format = rtDesc.Format;

		Microsoft::WRL::ComPtr<ID3D11Texture2D> tex;
		m_device->CreateTexture2D(&rtDesc, nullptr, tex.GetAddressOf());

		m_device->CreateRenderTargetView(tex.Get(), &rtvDesc, m_mrtRTVs[i].GetAddressOf());
		m_device->CreateShaderResourceView(tex.Get(), &srvDesc, m_mrtSRVs[i].GetAddressOf());
	}
}

//----------------------------------------------------
// Handles any fully pre-render tasks - clearing buffers,
// setting Multiple Render Targets, etc.
//----------------------------------------------------
void Renderer::FrameStart()
{
	// Clear the back buffer (erases what's on the screen)
	float bgColor[4] = { 0.4f, 0.6f, 0.75f, 1.0f }; // Cornflower Blue
	m_context->ClearRenderTargetView(m_backBufferRTV.Get(), bgColor);

	// Clear the depth buffer (resets per-pixel occlusion information)
	m_context->ClearDepthStencilView(m_depthBufferDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

	// Clear all MRT RTVs
	for (int i = 0; i < 4; i++) {
		bgColor[i] = 0.f;
	}
	for (Microsoft::WRL::ComPtr<ID3D11RenderTargetView>& rtv : m_mrtRTVs) {
		m_context->ClearRenderTargetView(rtv.Get(), bgColor);
	}
	bgColor[0] = 1.f; // Just clear Depth again. Not too much extra cost and I didn't want to do another check
	m_context->ClearRenderTargetView(m_mrtRTVs[RenderTarget::RT_SCENE_DEPTH].Get(), bgColor);

	// Prep MRTs for SPECIFICALLY SSAO
	// Loop through all assigned MRTs and get their pointers
	//	- Loop assumes that RTVs are assigned in a certain order, which
	//	  does not account for their Name. This is fine? since the names
	//	  are hardcoded into indices anyways, but still gives less control
	std::vector<ID3D11RenderTargetView*> targets;
	for (Microsoft::WRL::ComPtr<ID3D11RenderTargetView>& rtv : m_mrtRTVs) {
		targets.push_back(rtv.Get());
	}
	// If somehow there are more Render Targets, only set 8. This WILL set some unnecessary ones, but that is
	// fine if they are not rendered to
	m_context->OMSetRenderTargets((UINT)targets.size() <= 8 ? (UINT)targets.size() : 8, &targets[0], m_depthBufferDSV.Get());
}

//----------------------------------------------------
// Perform end-of frame tasks - drawing UI and Presenting
// the final frame
//	- vsync is more properly calculated by the Game
//	  information, so the final yes/no is passed in
//----------------------------------------------------
void Renderer::FrameEnd(bool a_vsync)
{
	// Draw ImGUI interface LAST
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	// Present the back buffer to the user
	//  - Puts the results of what we've drawn onto the window
	//  - Without this, the user never sees anything
	m_swapChain->Present(
		a_vsync ? 1 : 0,
		a_vsync ? 0 : DXGI_PRESENT_ALLOW_TEARING
	);

	// Must re-bind buffers after presenting, as they become unbound
	m_context->OMSetRenderTargets(1, m_backBufferRTV.GetAddressOf(), m_depthBufferDSV.Get());
}

//----------------------------------------------------
// Performs actual Scene render in all steps:
// Opaque, Sky, and Transparent (not implemented).
//	- Forward rendering
//	- Const references are passed in to prevent copying
//	  of potentially large data sets (this should be
//	  done in many other places as well)
//	- Could be optimized by passing in a single Scene
//	  object (also a const reference or ptr, since
//	  internally it would store lots of data)
//	- Many optimizations could be done in this step,
//	  including entity culling and sorting by Material
//	  to minimize data transfers
//		- For now is just a basic, brute force simple
//		  operation
//----------------------------------------------------
void Renderer::Render(
	const std::vector<std::shared_ptr<Entity>>& a_entities, 
	const std::vector<BasicLight>& a_allLights, 
	const std::shared_ptr<Camera>& a_camera, 
	const std::shared_ptr<Sky>& a_sky)
{
	// First sort Lights by type (passed in as a single array to minimize parameters)
	//	- Will be fixed by adding a Scene type
	std::vector<BasicLight> directionalLights;
	std::vector<BasicLight> pointLights;
	for (const BasicLight& light : a_allLights) {
		if (light.Type == LightType::Directional) {
			directionalLights.push_back(light);
		}
		else if (light.Type == LightType::Point) {
			pointLights.push_back(light);
		}
		// else do nothing (all other types) - no spot lights have been implemented YET
	}

	// Render all opaque entities (transparent entities don't exist, so opaque is everything)
	for (std::shared_ptr<Entity> entity : a_entities) {
		std::shared_ptr<SimplePixelShader> pixelShader = entity->GetMaterial()->GetPixelShader();

		// Set Light Data
		int directionalLightCount = (int)directionalLights.size();
		int pointLightCount = (int)pointLights.size();
		// Directionals
		if (pixelShader->HasVariable("c_directionalLights") && directionalLightCount > 0)
			pixelShader->SetData("c_directionalLights", &directionalLights[0], sizeof(BasicLight) * directionalLightCount);
		if (pixelShader->HasVariable("c_directionalLightCount"))
			pixelShader->SetInt("c_directionalLightCount", directionalLightCount);
		// Points
		if (pixelShader->HasVariable("c_pointLights") && pointLightCount > 1)
			pixelShader->SetData("c_pointLights", &pointLights[0], sizeof(BasicLight) * pointLightCount);
		if (pixelShader->HasVariable("c_pointLightCount"))
			pixelShader->SetInt("c_pointLightCount", pointLightCount);

		// Set IBL Maps
		if (pixelShader->HasShaderResourceView("IrradianceMap"))
			pixelShader->SetShaderResourceView("IrradianceMap", a_sky->GetEnvironmentMap());
		if (pixelShader->HasShaderResourceView("ReflectionMap"))
			pixelShader->SetShaderResourceView("ReflectionMap", a_sky->GetReflectanceMap());
		if (pixelShader->HasShaderResourceView("BRDFIntegrationMap"))
			pixelShader->SetShaderResourceView("BRDFIntegrationMap", m_iblBRDFLookupTexture);

		// Draw Entity
		entity->Draw(m_context, a_camera);
	}

	// Draw Sky after all Entities
	a_sky->Draw(m_context, a_camera);

	// This is where Transparent object rendering and particles would go

	// Reset Render Targets to Null for post-processing - all 8 bound ones (whatever they are)
	ID3D11RenderTargetView* nulls[8] = {};
	m_context->OMSetRenderTargets(8, nulls, m_depthBufferDSV.Get());
}

//----------------------------------------------------
// Perform any post-process steps before sending final
// data to Back Buffer and Presenting
//	- Accepts a Camera for use in SSAO calculation
//----------------------------------------------------
void Renderer::PostProcess(std::shared_ptr<Camera> a_camera)
{
	DisplayRenderTargets({ RT_SCENE_COLOR, RT_SCENE_AMBIENT, RT_SCENE_NORMAL, RT_SCENE_DEPTH });

	// Perform initial SSAO pass
	{
		m_postProcessVS->SetShader();
		m_ssaoCorePS->SetShader();

		// Set data for core SSAO pass
		Matrix4 projMatrix = a_camera->GetProjectionMatrix();
		Matrix4 invProj;
		XMStoreFloat4x4(&invProj, XMMatrixInverse(nullptr, XMLoadFloat4x4(&projMatrix)));

		if (m_ssaoCorePS->HasVariable("c_viewMatrix"))
			m_ssaoCorePS->SetMatrix4x4("c_viewMatrix", a_camera->GetViewMatrix());
		if (m_ssaoCorePS->HasVariable("c_projectionMatrix"))
			m_ssaoCorePS->SetMatrix4x4("c_projectionMatrix", projMatrix);
		if (m_ssaoCorePS->HasVariable("c_inverseProjMatrix"))
			m_ssaoCorePS->SetMatrix4x4("c_inverseProjMatrix", invProj);
		if (m_ssaoCorePS->HasVariable("c_offsets"))
			m_ssaoCorePS->SetData("c_offsets", &m_ssaoOffsets[0], (int)m_ssaoOffsets.size() * sizeof(Vector4));
		if (m_ssaoCorePS->HasVariable("c_radius"))
			m_ssaoCorePS->SetFloat("c_radius", 1.f);
		if (m_ssaoCorePS->HasVariable("c_samples"))
			m_ssaoCorePS->SetInt("c_samples", (int)m_ssaoOffsets.size()); // CANNOT exceed 64
		if (m_ssaoCorePS->HasVariable("c_randomSampleScreenScale"))
			m_ssaoCorePS->SetFloat2("c_randomSampleScreenScale", Vector2((float)m_windowWidth / 4.f, (float)m_windowHeight / 4.f));
			// The random texture has a size of 4 in each dimension - not worth saving in class but may be worth a #define

		// Set Samplers (no need to store these, since they'll remain bound and are functionally the same?
		//	- Sort of. Anisotropic filtering is not required
		if (m_ssaoCorePS->HasSamplerState("BasicSampler"))
			m_ssaoCorePS->SetSamplerState("BasicSampler", m_standardSampler);
		if (m_ssaoCorePS->HasSamplerState("ClampSampler"))
			m_ssaoCorePS->SetSamplerState("ClampSampler", m_clampSampler);

		// Set SRVs
		if (m_ssaoCorePS->HasShaderResourceView("Random"))
			m_ssaoCorePS->SetShaderResourceView("Random", m_ssaoRandomOffsets);
		if (m_ssaoCorePS->HasShaderResourceView("SceneNormals"))
			m_ssaoCorePS->SetShaderResourceView("SceneNormals", m_mrtSRVs[RT_SCENE_NORMAL]);
		if (m_ssaoCorePS->HasShaderResourceView("SceneDepths"))
			m_ssaoCorePS->SetShaderResourceView("SceneDepths", m_mrtSRVs[RT_SCENE_DEPTH]);

		m_ssaoCorePS->CopyAllBufferData();

		// Set Render Target to SSAO result and Draw
		m_context->OMSetRenderTargets(1, m_mrtRTVs[RT_POST_PROCESS_ZERO].GetAddressOf(), nullptr);
		m_context->Draw(3, 0);
	}

	DisplayRenderTargets({ RT_POST_PROCESS_ZERO }); // Display in debugger the newly rendered SSAO texture

	// Perform Blur pass to reduce pattern
	{
		// Set render target first to unbind former one and allow binding as SRV
		m_context->OMSetRenderTargets(1, m_mrtRTVs[RT_POST_PROCESS_ONE].GetAddressOf(), nullptr);

		m_ssaoBlurPS->SetShader();

		// Set cbuffer data and resources
		if (m_ssaoBlurPS->HasVariable("c_pixelSize"))
			m_ssaoBlurPS->SetFloat2("c_pixelSize", Vector2(1.f / (float)m_windowWidth, 1.f / (float)m_windowHeight));
		if (m_ssaoBlurPS->HasSamplerState("ClampSampler"))
			m_ssaoBlurPS->SetSamplerState("ClampSampler", m_clampSampler);
		if (m_ssaoBlurPS->HasShaderResourceView("BlurTarget"))
			m_ssaoBlurPS->SetShaderResourceView("BlurTarget", m_mrtSRVs[RT_POST_PROCESS_ZERO]);

		m_ssaoBlurPS->CopyAllBufferData();
		m_context->Draw(3, 0);
	}

	DisplayRenderTargets({ RT_POST_PROCESS_ONE }); // Display blurred result

	// Perform final combination pass to occlude ambient light
	{
		m_context->OMSetRenderTargets(1, m_backBufferRTV.GetAddressOf(), nullptr);

		m_ssaoCombinePS->SetShader();

		// Set Resources
		if (m_ssaoCombinePS->HasShaderResourceView("SceneColors"))
			m_ssaoCombinePS->SetShaderResourceView("SceneColors", m_mrtSRVs[RT_SCENE_COLOR]);
		if (m_ssaoCombinePS->HasShaderResourceView("SceneAmbient"))
			m_ssaoCombinePS->SetShaderResourceView("SceneAmbient", m_mrtSRVs[RT_SCENE_AMBIENT]);
		if (m_ssaoCombinePS->HasShaderResourceView("SceneDepths"))
			m_ssaoCombinePS->SetShaderResourceView("SceneDepths", m_mrtSRVs[RT_SCENE_DEPTH]);
		if (m_ssaoCombinePS->HasShaderResourceView("SSAO"))
			m_ssaoCombinePS->SetShaderResourceView("SSAO", m_mrtSRVs[RT_POST_PROCESS_ONE]);
		if (m_ssaoCombinePS->HasSamplerState("ClampSampler"))
			m_ssaoCombinePS->SetSamplerState("ClampSampler", m_clampSampler);

		// No data to copy in cbuffers, so just Draw
		m_context->Draw(3, 0);
	}

	// Rebind Back Buffer and unbind all possible SRV slots (so they can be rebound as inputs next frame)
	//m_context->OMSetRenderTargets(1, m_backBufferRTV.GetAddressOf(), m_depthBufferDSV.Get());
	ID3D11ShaderResourceView* srvs[128] = {}; // 128 is all of them without guessing a size
	m_context->PSSetShaderResources(0, 128, srvs);
}

//----------------------------------------------------
// Draw a set of Render Targets from the MRT setup attached
// to this Renderer through ImGUI
//	- Would be nice to display horizontally
//----------------------------------------------------
void Renderer::DisplayRenderTargets(std::vector<RenderTarget> a_rtIndices)
{
	ImGui::Begin("MRT Displays");
	for (RenderTarget rt : a_rtIndices) {
		ImGui::Image(m_mrtSRVs[rt].Get(), ImVec2(m_windowWidth / 4.f , m_windowHeight / 4.f));
	}
	ImGui::End();
}
