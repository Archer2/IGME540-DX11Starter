#include "ReflectionProbe.h"

// Defines for static sizes of texture maps
#define REFLECTION_MAP_SIZE 128 // Should be very small
#define SCENE_MAP_SIZE 512 // Can still be small (I hope)

//---------------------------------------------------------
// Fill in standard values and build initial GPU resources
//---------------------------------------------------------
ReflectionProbe::ReflectionProbe(float a_radius, Vector3 a_position, std::shared_ptr<SimpleVertexShader> a_sceneVS, std::shared_ptr<SimpleVertexShader> a_refVS, std::shared_ptr<SimplePixelShader> a_scenePS, std::shared_ptr<SimplePixelShader> a_refPS, Microsoft::WRL::ComPtr<ID3D11Device> a_d3dDevice)
	: m_radius(a_radius)
	, m_position(a_position)
	, m_sceneVertexShader(a_sceneVS)
	, m_reflectionVertexShader(a_refVS)
	, m_scenePixelShader(a_scenePS)
	, m_reflectionPixelShader(a_refPS)
{
	BuildResources(a_d3dDevice);
}

//---------------------------------------------------------
// Drawing the real-time ReflectionProbe data takes place in
// two steps. The first is to render the Scene to an image,
// then the second is to use that image as an environment
// from which to render a convolved reflection map
//	- Passing in Scene data (lights, entities, sky, etc.) is
//	  done with const references to avoid copying large vectors
//		- This should be done elsewhere in code as well
//	- Scene data would be vastly simplified by having a
//	  unified Scnee class - pass in the instance instead of
//	  tons of individual vectors
//---------------------------------------------------------
void ReflectionProbe::Draw(Microsoft::WRL::ComPtr<ID3D11Device> a_d3dDevice, Microsoft::WRL::ComPtr<ID3D11DeviceContext> a_d3dContext, const std::vector<std::shared_ptr<Entity>>& a_entities, const std::vector<BasicLight>& a_directionalLights, const std::vector<BasicLight>& a_pointLights, const std::shared_ptr<Sky> a_sky, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> a_brdfLookUp)
{
	// Store/destroy currently active Render Targets and Input buffers
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> cachedRTV;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> cachedDSV;
	a_d3dContext->OMGetRenderTargets(1, cachedRTV.GetAddressOf(), cachedDSV.GetAddressOf());
	UINT viewportCount = 1;
	D3D11_VIEWPORT cachedViewport;
	a_d3dContext->RSGetViewports(&viewportCount, &cachedViewport);
	a_d3dContext->IASetIndexBuffer(nullptr, DXGI_FORMAT_R32_UINT, 0);
	UINT stride = sizeof(Vertex); // Stride of everything else being drawn?
	UINT offset = 0;
	ID3D11Buffer* nullBuffer = nullptr; // Must pass address of nullptr to clear, not nullptr itself
	a_d3dContext->IASetVertexBuffers(0, 1, &nullBuffer, &stride, &offset);

	// Prepare a Camera through which to render the Scene
	Transform t = Transform::ZeroTransform;
	t.SetAbsolutePosition(m_position);
	std::shared_ptr<Camera> cam = std::make_shared<Camera>(t, DirectX::XMINT2(SCENE_MAP_SIZE, SCENE_MAP_SIZE));

	// Set Shaders
	m_sceneVertexShader->SetShader();
	m_scenePixelShader->SetShader();

	// Render the scene 6 times
	//	- This does not fully work - camera is not actually rotated for each face
	for (int i = 0; i < 6; i++) {
		// Adjust Camera
		//	- This fails - it renders facing forward for all sides
		switch (i) {
		case 0: cam->SetCameraRotation(0.f, DirectX::XM_PIDIV2, 0.f); break;
		case 1: cam->SetCameraRotation(0.f, -DirectX::XM_PIDIV2, 0.f); break;
		case 2: cam->SetCameraRotation(-DirectX::XM_PIDIV2, 0.f, 0.f); break;
		case 3: cam->SetCameraRotation(DirectX::XM_PIDIV2, 0.f, 0.f); break;
		case 4: cam->SetCameraRotation(0.f, 0.f, 0.f); break;
		case 5: cam->SetCameraRotation(0.f, DirectX::XM_PI, 0.f); break;
		}

		// Create RTV
		D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
		rtvDesc.Texture2DArray.MipSlice = 0;
		rtvDesc.Texture2DArray.FirstArraySlice = i; // +X -> -X -> +Y -> -Y -> +Z -> -Z
		rtvDesc.Texture2DArray.ArraySize = 1;
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> faceRTV;
		a_d3dDevice->CreateRenderTargetView(m_sceneMapTexture.Get(), &rtvDesc, faceRTV.GetAddressOf());
		a_d3dContext->OMSetRenderTargets(1, faceRTV.GetAddressOf(), cachedDSV.Get());

		// Clear newly set targets	
		const float bgColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
		a_d3dContext->ClearRenderTargetView(faceRTV.Get(), bgColor);
		a_d3dContext->ClearDepthStencilView(cachedDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

		// Actually Render Scene to this section of the cubemap
		RenderScene(faceRTV, a_d3dContext, a_entities, a_directionalLights, a_pointLights, a_sky, cam, a_brdfLookUp);
	}

	// TODO: Render Convolved Reflection Map using the newly rendered out Scene Map as the Environment to convolve

	// Restore cached viewport, RTV, and DSV (Input buffers are the next object's responsibility
	a_d3dContext->OMSetRenderTargets(1, cachedRTV.GetAddressOf(), cachedDSV.Get());
	a_d3dContext->RSSetViewports(1, &cachedViewport);
}

//---------------------------------------------------------
// Builds the two cubemaps on the GPU. Textures must also be
// stored, since the underlying resource will need to be
// quickly extracted to generate RTVs every frame
//---------------------------------------------------------
void ReflectionProbe::BuildResources(Microsoft::WRL::ComPtr<ID3D11Device> a_d3dDevice)
{
	// Create reflection map texture
	int ignoredSmallMips = 2; // 2x2 and 4x4 are irrelevant
	int mipLevels = max((int)(log2(REFLECTION_MAP_SIZE) - ignoredSmallMips), 1); // Subtract ignored Mip levels from the total count
	D3D11_TEXTURE2D_DESC texDesc = {};
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.Width = REFLECTION_MAP_SIZE;
	texDesc.Height = texDesc.Width;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	texDesc.MipLevels = mipLevels;
	texDesc.ArraySize = 6; // TextureCube
	texDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
	texDesc.SampleDesc.Count = 1;
	a_d3dDevice->CreateTexture2D(&texDesc, nullptr, m_reflectionMapTexture.GetAddressOf());

	// Create scene cube texture
	texDesc.Width = SCENE_MAP_SIZE;
	texDesc.Height = texDesc.Width;
	texDesc.MipLevels = 1;
	a_d3dDevice->CreateTexture2D(&texDesc, nullptr, m_sceneMapTexture.GetAddressOf());

	// Create SRV for Reflection Map
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = texDesc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.TextureCube.MipLevels = mipLevels;
	srvDesc.TextureCube.MostDetailedMip = 0;
	a_d3dDevice->CreateShaderResourceView(m_reflectionMapTexture.Get(), &srvDesc, m_reflectionMap.GetAddressOf());

	// Create SRV for Scene Map
	srvDesc.TextureCube.MipLevels = 1;
	a_d3dDevice->CreateShaderResourceView(m_sceneMapTexture.Get(), &srvDesc, m_sceneCubeMap.GetAddressOf());
}

//---------------------------------------------------------
// Takes care of the actual Render Loop for a single RTV
//	- One face of the Scene Cubemap
//	- Render loop is taken directly from Game::Draw()
//---------------------------------------------------------
void ReflectionProbe::RenderScene(Microsoft::WRL::ComPtr<ID3D11RenderTargetView> a_rtv, Microsoft::WRL::ComPtr<ID3D11DeviceContext> a_d3dContext, const std::vector<std::shared_ptr<Entity>>& a_entities, const std::vector<BasicLight>& a_directionalLights, const std::vector<BasicLight>& a_pointLights, const std::shared_ptr<Sky> a_sky, std::shared_ptr<Camera> a_camera, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> a_brdfLookUp)
{
	for (std::shared_ptr<Entity> entity : a_entities) {
		std::shared_ptr<SimplePixelShader> pixelShader = entity->GetMaterial()->GetPixelShader();
		int directionalLightCount = (int)a_directionalLights.size();
		int pointLightCount = (int)a_pointLights.size();

		if (pixelShader->HasVariable("c_directionalLights") && directionalLightCount > 0)
			pixelShader->SetData("c_directionalLights", &a_directionalLights[0], sizeof(BasicLight) * directionalLightCount);
		if (pixelShader->HasVariable("c_directionalLightCount"))
			pixelShader->SetInt("c_directionalLightCount", directionalLightCount);
		if (pixelShader->HasVariable("c_pointLights") && pointLightCount > 1)
			pixelShader->SetData("c_pointLights", &a_pointLights[0], sizeof(BasicLight) * pointLightCount);
		if (pixelShader->HasVariable("c_pointLightCount"))
			pixelShader->SetInt("c_pointLightCount", pointLightCount);
		if (pixelShader->HasShaderResourceView("IrradianceMap"))
			pixelShader->SetShaderResourceView("IrradianceMap", a_sky->GetEnvironmentMap());
		if (pixelShader->HasShaderResourceView("ReflectionMap"))
			pixelShader->SetShaderResourceView("ReflectionMap", a_sky->GetReflectanceMap());
		if (pixelShader->HasShaderResourceView("BRDFIntegrationMap"))
			pixelShader->SetShaderResourceView("BRDFIntegrationMap", a_brdfLookUp);

		entity->Draw(a_d3dContext, a_camera);
	}
	a_sky->Draw(a_d3dContext, a_camera);
}
