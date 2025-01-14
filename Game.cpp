#include "Game.h"
#include "Vertex.h"
#include "Input.h"
#include "Helpers.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx11.h"

// Needed for a helper function to load pre-compiled shader files
#pragma comment(lib, "d3dcompiler.lib")
#include <d3dcompiler.h>
#include <ctime> // For seeding C Random generator with current time

#include "WICTextureLoader.h"
#include "DDSTextureLoader.h"

// For the DirectX Math library
using namespace DirectX;

// --------------------------------------------------------
// Constructor
//
// DXCore (base class) constructor will set up underlying fields.
// Direct3D itself, and our window, are not ready at this point!
//
// hInstance - the application's OS-level handle (unique ID)
// --------------------------------------------------------
Game::Game(HINSTANCE hInstance)
	: DXCore(
		hInstance,			// The application's handle
		L"DirectX Game",	// Text for the window's title bar (as a wide-character string)
		1280,				// Width of the window's client area
		720,				// Height of the window's client area
		false,				// Sync the framerate to the monitor refresh? (lock framerate)
		true)				// Show extra stats (fps) in title bar?
{
#if defined(DEBUG) || defined(_DEBUG)
	// Do we want a console window?  Probably only in debug mode
	CreateConsoleWindow(500, 120, 32, 120);
	printf("Console window created successfully.  Feel free to printf() here.\n");
#endif
}

// --------------------------------------------------------
// Destructor - Clean up anything our game has created:
//  - Delete all objects manually created within this class
//  - Release() all Direct3D objects created within this class
// --------------------------------------------------------
Game::~Game()
{
	// Call delete or delete[] on any objects or arrays you've
	// created using new or new[] within this class
	// - Note: this is unnecessary if using smart pointers

	// Call Release() on any Direct3D objects made within this class
	// - Note: this is unnecessary for D3D objects stored in ComPtrs

	// Clean up ImGUI
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

// --------------------------------------------------------
// Called once per program, after Direct3D and the window
// are initialized but before the game loop.
// --------------------------------------------------------
void Game::Init()
{
	// Seed Standard Random Number generator
	std::srand((unsigned int)std::time(0));

	// Tell the input assembler (IA) stage of the pipeline what kind of
	// geometric primitives (points, lines or triangles) we want to draw.  
	// Essentially: "What kind of shape should the GPU draw with our vertices?"
	//	- Moved here to be set before the Skybox is generated, for Irradiance calculation
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Set up ImGUI
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplWin32_Init(hWnd);
	ImGui_ImplDX11_Init(device.Get(), context.Get());
	ImGui::StyleColorsClassic(); // Or Dark or Light

	// Helper methods for loading shaders, creating some basic
	// geometry to draw and some simple camera matrices.
	//  - You'll be expanding and/or replacing these later
	LoadShaders();
	CreateMaterials();
	LoadGeometry();
	GenerateEntities();
	CreateLights();

	// Create Renderer (MUST be done after LoadShaders so BRDF Texture is loaded
	m_renderer = std::make_shared<Renderer>(device, context, swapChain, backBufferRTV, 
		depthBufferDSV, iblBRDFLookupTexture, fullscreenTriangleVertexShader, windowWidth, windowHeight);

	// Test a Reflection Probe
	//std::shared_ptr<ReflectionProbe> probe = std::make_shared<ReflectionProbe>(100.f, Vector3(0.f, 0.f, -10.f), vertexShader, fullscreenTriangleVertexShader, pixelShader, envPrefilterPixelShader, device);
	//reflectionProbes.push_back(probe);
	
	// Create Camera some units behind the origin
	Transform cameraTransform = Transform::ZeroTransform;
	cameraTransform.SetAbsolutePosition(0.f, 1.5f, -10.f); // Rearranged for Final demo scene
	//cameraTransform.SetAbsoluteRotation(0.f, 0.f, XM_PI); // Does not work, because Camera still uses hacked rotation
	camera = std::make_shared<Camera>(cameraTransform, XMINT2(this->windowWidth, this->windowHeight));
	//camera->AddCameraRotation(0.f, XM_PI, 0.f); // Hacked Camera rotation
	//camera->AddCameraRotation(DirectX::XMConvertToRadians(12.f), 0.f, 0.f); // Final Demo scene camera pos
}

// --------------------------------------------------------
// Loads shaders from compiled shader object (.cso) files
// and also created the Input Layout that describes our 
// vertex data to the rendering pipeline. 
// - Input Layout creation is done here because it must 
//    be verified against vertex shader byte code
// - We'll have that byte code already loaded below
// --------------------------------------------------------
void Game::LoadShaders()
{
	vertexShader = std::make_shared<SimpleVertexShader>(device, context, FixPath(L"VertexShader.cso").c_str());
	pixelShader = std::make_shared<SimplePixelShader>(device, context, FixPath(L"PixelShader.cso").c_str());
	customPixelShader = std::make_shared<SimplePixelShader>(device, context, FixPath(L"ProceduralPixelShader.cso").c_str());
	skyVertexShader = std::make_shared<SimpleVertexShader>(device, context, FixPath(L"SkyVertexShader.cso").c_str());
	skyPixelShader = std::make_shared<SimplePixelShader>(device, context, FixPath(L"SkyPixelShader.cso").c_str());
	fullscreenTriangleVertexShader = std::make_shared<SimpleVertexShader>(device, context, FixPath(L"FullscreenTriangleVS.cso").c_str());
	irradiancePixelShader = std::make_shared<SimplePixelShader>(device, context, FixPath(L"IBLIrradianceMapPS.cso").c_str());
	envPrefilterPixelShader = std::make_shared<SimplePixelShader>(device, context, FixPath(L"IBLSpecularPrefilterPS.cso").c_str());
	brdfLookupMapPixelShader = std::make_shared<SimplePixelShader>(device, context, FixPath(L"IBlBRDFIntegrateMapPS.cso").c_str());

	// Create Shader resources univeral to the Game
	CreateIBLBRDFLookupTable();
}

// --------------------------------------------------------
// Loads basic geometry files
// --------------------------------------------------------
void Game::LoadGeometry()
{
	// Load default files provided in A6
	geometry.push_back(std::make_shared<Mesh>(FixPath(L"../../assets/meshes/cube.obj").c_str(), device, context));
	geometry.push_back(std::make_shared<Mesh>(FixPath(L"../../assets/meshes/cylinder.obj").c_str(), device, context));
	geometry.push_back(std::make_shared<Mesh>(FixPath(L"../../assets/meshes/helix.obj").c_str(), device, context));
	geometry.push_back(std::make_shared<Mesh>(FixPath(L"../../assets/meshes/sphere.obj").c_str(), device, context));
	geometry.push_back(std::make_shared<Mesh>(FixPath(L"../../assets/meshes/torus.obj").c_str(), device, context));
	geometry.push_back(std::make_shared<Mesh>(FixPath(L"../../assets/meshes/quad.obj").c_str(), device, context));
	geometry.push_back(std::make_shared<Mesh>(FixPath(L"../../assets/meshes/quad_double_sided.obj").c_str(), device, context));
}

// --------------------------------------------------------
// Generate a bunch of Entities using our stored geometry
//	- Entites all reference the same geometry
//	- This is what is actually drawn
// --------------------------------------------------------
void Game::GenerateEntities()
{
	// Create a Sky
	D3D11_SAMPLER_DESC desc = {};
	desc.Filter = D3D11_FILTER_MINIMUM_MIN_MAG_LINEAR_MIP_POINT; // Skybox is always same distance, so Point MIP, and Linear Color
	desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	desc.MaxLOD = D3D11_FLOAT32_MAX;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerState;
	device->CreateSamplerState(&desc, samplerState.GetAddressOf());
	sky = std::make_shared<Sky>(
		device,
		geometry[0],
		LoadTextureCube(L"../../assets/materials/skies/Clouds Blue"),
		samplerState,
		skyVertexShader,
		skyPixelShader);
	sky->CreateEnvironmentMap(device, context, fullscreenTriangleVertexShader, irradiancePixelShader);
	sky->CreateSpecularReflectanceMap(device, context, fullscreenTriangleVertexShader, envPrefilterPixelShader);

	// Generate a fancy cube just above world origin
	//entities.push_back(std::make_shared<Entity>(geometry[0], materials[materials.size()-1]));
	//entities[0]->GetTransform()->AddAbsolutePosition(0.f, 3.f, 0.f);
	//entities[0]->GetTransform()->SetAbsoluteRotation(0.f, XM_PIDIV4, XM_PIDIV4);

	// Generate a line of entities, 1 for each geometry or material
	float entityOffset = 2.f; // Offset of each entity from its neighbors
	// MODIFIED to cut off last 12 materials, since they are full non-metal and full metal with white color and flat normals
	float xPosition = (materials.size() - 13) * -(entityOffset / 2.f) - (entityOffset / 2.f); // Position of 1st entity for centered line
	for (int i = 0; i < materials.size()-13; i++) {
		xPosition += entityOffset; // Increment position for the line
	
		// Create and edit entity
		//std::shared_ptr<Entity> entity = std::make_shared<Entity>(geometry[i], materials[(UINT)GenerateRandomFloat(0.f, (float)materials.size()-1.f)]);
		std::shared_ptr<Entity> entity = std::make_shared<Entity>(geometry[3], materials[i]);
		entity->GetTransform()->SetAbsolutePosition(xPosition, 0.f, 0.f); // Offset down so planes are visible from origin camera
		entities.push_back(entity);
	}
	// Create upper and lower rows of IBL Demo spheres
	xPosition = 6.f * -(entityOffset / 2.f) - (entityOffset / 2.f);
	for (size_t i = materials.size() - 13; i < materials.size() - 7; i++) {
		xPosition += entityOffset;
		std::shared_ptr<Entity> entity = std::make_shared<Entity>(geometry[3], materials[i]);
		entity->GetTransform()->SetAbsolutePosition(xPosition, entityOffset, 0.f);
		entities.push_back(entity);
	}
	xPosition = 6.f * -(entityOffset / 2.f) - (entityOffset / 2.f);
	for (size_t i = materials.size() - 7; i < materials.size() - 1; i++) {
		xPosition += entityOffset;
		std::shared_ptr<Entity> entity = std::make_shared<Entity>(geometry[3], materials[i]);
		entity->GetTransform()->SetAbsolutePosition(xPosition, -entityOffset, 0.f);
		entities.push_back(entity);
	}

	// Create Entities for a screen of a simple room and table - Final Demo (IGME 540)
	/*
	{
		int counter = 0; // Counter for current Entity
		// Floor
		entities.push_back(std::make_shared<Entity>(geometry[3], materials[3]));
		entities[counter]->GetTransform()->AddAbsolutePosition(0.f, -1.f, 0.f);
		entities[counter]->GetTransform()->SetAbsoluteScale(3.25f, 1.f, 4.25f);

		// Columns
		counter++;
		entities.push_back(std::make_shared<Entity>(geometry[1], materials[5]));
		entities[counter]->GetTransform()->SetAbsoluteScale(.2f, 1.25f, .2f);
		entities[counter]->GetTransform()->SetAbsolutePosition(3.f, .25f, -4.f);
		counter++;
		entities.push_back(std::make_shared<Entity>(geometry[1], materials[5]));
		entities[counter]->GetTransform()->SetAbsoluteScale(.2f, 1.25f, .2f);
		entities[counter]->GetTransform()->SetAbsolutePosition(-3.f, .25f, -4.f);
		counter++;
		entities.push_back(std::make_shared<Entity>(geometry[1], materials[5]));
		entities[counter]->GetTransform()->SetAbsoluteScale(.2f, 1.25f, .2f);
		entities[counter]->GetTransform()->SetAbsolutePosition(3.f, .25f, 4.f);
		counter++;
		entities.push_back(std::make_shared<Entity>(geometry[1], materials[5]));
		entities[counter]->GetTransform()->SetAbsoluteScale(.2f, 1.25f, .2f);
		entities[counter]->GetTransform()->SetAbsolutePosition(-3.f, .25f, 4.f);

		// Spheres
		counter++;
		entities.push_back(std::make_shared<Entity>(geometry[2], materials[4]));
		entities[counter]->GetTransform()->SetAbsoluteScale(.4f, .4f, .4f);
		entities[counter]->GetTransform()->SetAbsolutePosition(3.f, 1.75f, -4.f);
		counter++;
		entities.push_back(std::make_shared<Entity>(geometry[2], materials[4]));
		entities[counter]->GetTransform()->SetAbsoluteScale(.4f, .4f, .4f);
		entities[counter]->GetTransform()->SetAbsolutePosition(-3.f, 1.75f, -4.f);
		counter++;
		entities.push_back(std::make_shared<Entity>(geometry[2], materials[4]));
		entities[counter]->GetTransform()->SetAbsoluteScale(.4f, .4f, .4f);
		entities[counter]->GetTransform()->SetAbsolutePosition(3.f, 1.75f, 4.f);
		counter++;
		entities.push_back(std::make_shared<Entity>(geometry[2], materials[4]));
		entities[counter]->GetTransform()->SetAbsoluteScale(.4f, .4f, .4f);
		entities[counter]->GetTransform()->SetAbsolutePosition(-3.f, 1.75f, 4.f);

		// Table
		counter++;
		entities.push_back(std::make_shared<Entity>(geometry[1], materials[1]));
		entities[counter]->GetTransform()->SetAbsoluteScale(.1f, .25f, .1f);
		entities[counter]->GetTransform()->SetAbsolutePosition(1.f, -.75f, -1.5f);
		counter++;
		entities.push_back(std::make_shared<Entity>(geometry[1], materials[1]));
		entities[counter]->GetTransform()->SetAbsoluteScale(.1f, .25f, .1f);
		entities[counter]->GetTransform()->SetAbsolutePosition(-1.f, -.75f, -1.5f);
		counter++;
		entities.push_back(std::make_shared<Entity>(geometry[1], materials[1]));
		entities[counter]->GetTransform()->SetAbsoluteScale(.1f, .25f, .1f);
		entities[counter]->GetTransform()->SetAbsolutePosition(1.f, -.75f, 1.5f);
		counter++;
		entities.push_back(std::make_shared<Entity>(geometry[1], materials[1]));
		entities[counter]->GetTransform()->SetAbsoluteScale(.1f, .25f, .1f);
		entities[counter]->GetTransform()->SetAbsolutePosition(-1.f, -.75f, 1.5f);
		counter++;
		entities.push_back(std::make_shared<Entity>(geometry[0], materials[0]));
		entities[counter]->GetTransform()->SetAbsoluteScale(1.3f, .03f, 1.8f);
		entities[counter]->GetTransform()->SetAbsolutePosition(0.f, -.5f, 0.f);
	}
	*/
}

// ----------------------------------------------------------
// Initializes some Materials using loaded in textures
// ----------------------------------------------------------
void Game::CreateMaterials()
{
	// Default Textures
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> defaultNormalSRV =
		LoadTexture(L"../../assets/materials/flat_normals.png");
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> fullNonMetalSRV =
		LoadTexture(L"../../assets/materials/no_metal.png");
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> fullMetalSRV =
		LoadTexture(L"../../assets/materials/full_metal.png");

	// Textures from AmbientCG.com
	// - They have 2 normal maps - GL and DX. I assume that is OpenGL vs. DirectX for handedness, since the normals appear inverted
	// - Surface color is NOT gamma-corrected, so "reversing" the auto gamma correction in the PS after sampling just makes the
	//	 texture darker
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> marbleSRV =
		LoadTexture(L"../../assets/materials/Marble023_1K/Marble023_1K_Color.png");
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> marbleNormalSRV =
		LoadTexture(L"../../assets/materials/Marble023_1K/Marble023_1K_NormalDX.png"); 
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> marbleRoughnessSRV =
		LoadTexture(L"../../assets/materials/Marble023_1K/Marble023_1K_Roughness.png");
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> marbleMetalnessSRV =
		LoadTexture(L"../../assets/materials/Marble023_1K/Marble023_1K_Metalness.png");

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> metalPlatesSRV =
		LoadTexture(L"../../assets/materials/MetalPlates006_1K/MetalPlates006_1K_Color.png");
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> metalPlatesNormalSRV =
		LoadTexture(L"../../assets/materials/MetalPlates006_1K/MetalPlates006_1K_NormalDX.png"); 
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> metalPlatesRoughnessSRV =
		LoadTexture(L"../../assets/materials/MetalPlates006_1K/MetalPlates006_1K_Roughness.png");
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> metalPlatesMetalnessSRV =
		LoadTexture(L"../../assets/materials/MetalPlates006_1K/MetalPlates006_1K_Metalness.png");

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> woodSRV = 
		LoadTexture(L"../../assets/materials/Wood058_1K/Wood058_1K_Color.png");
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> woodNormalSRV =
		LoadTexture(L"../../assets/materials/Wood058_1K/Wood058_1K_NormalDX.png"); 
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> woodRoughnessSRV =
		LoadTexture(L"../../assets/materials/Wood058_1K/Wood058_1K_Roughness.png");
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> woodMetalnessSRV =
		LoadTexture(L"../../assets/materials/Wood058_1K/Wood058_1K_Metalness.png");

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> metalSRV =
		LoadTexture(L"../../assets/materials/Metal032_1K/Metal032_1K_Color.png");
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> metalNormalSRV =
		LoadTexture(L"../../assets/materials/Metal032_1K/Metal032_1K_NormalDX.png");
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> metalRoughnessSRV =
		LoadTexture(L"../../assets/materials/Metal032_1K/Metal032_1K_Roughness.png");
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> metalMetalnessSRV =
		LoadTexture(L"../../assets/materials/Metal032_1K/Metal032_1K_Metalness.png");

	// Provided PBR textures
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> cobbleSRV =
		LoadTexture(L"../../assets/materials/Cobblestone/cobblestone_albedo.png");
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> cobbleNormalSRV =
		LoadTexture(L"../../assets/materials/Cobblestone/cobblestone_normals.png");
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> cobbleRoughnessSRV =
		LoadTexture(L"../../assets/materials/Cobblestone/cobblestone_roughness.png");
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> cobbleMetalnessSRV =
		LoadTexture(L"../../assets/materials/Cobblestone/cobblestone_metal.png");

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> bronzeSRV =
		LoadTexture(L"../../assets/materials/Bronze/bronze_albedo.png");
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> bronzeNormalsSRV =
		LoadTexture(L"../../assets/materials/Bronze/bronze_normals.png");
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> bronzeRoughnessSRV =
		LoadTexture(L"../../assets/materials/Bronze/bronze_roughness.png");
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> bronzeMetalnessSRV =
		LoadTexture(L"../../assets/materials/Bronze/bronze_metal.png");

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> paintSRV =
		LoadTexture(L"../../assets/materials/Scratched/scratched_albedo.png");
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> paintNormalsSRV =
		LoadTexture(L"../../assets/materials/Scratched/scratched_normals.png");
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> paintRoughnessSRV =
		LoadTexture(L"../../assets/materials/Scratched/scratched_roughness.png");
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> paintMetalnessSRV =
		LoadTexture(L"../../assets/materials/Scratched/scratched_metal.png");

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> floorSRV =
		LoadTexture(L"../../assets/materials/Floor/floor_albedo.png");
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> floorNormalSRV =
		LoadTexture(L"../../assets/materials/Floor/floor_normals.png");
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> floorRoughnessSRV =
		LoadTexture(L"../../assets/materials/Floor/floor_roughness.png");
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> floorMetalnessSRV =
		LoadTexture(L"../../assets/materials/Floor/floor_metal.png");

	// Create a Sampler state
	D3D11_SAMPLER_DESC desc = {};
	desc.Filter = D3D11_FILTER_ANISOTROPIC;
	desc.MaxAnisotropy = 4;
	desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	desc.MaxLOD = D3D11_FLOAT32_MAX;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerState;
	device->CreateSamplerState(&desc, samplerState.GetAddressOf());

	// Create a Clamped version as well
	desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> clampState;
	device->CreateSamplerState(&desc, clampState.GetAddressOf());

	// Basic Pixel and Vertex Shader Materials (basic white color tint)
	size_t counter = materials.size(); // Counter to access the materials vector at the new slot

	// Marble
	materials.push_back(std::make_shared<Material>(vertexShader, pixelShader, XMFLOAT4(1.f, 1.f, 1.f, 1.f), 0.f));
	materials[counter]->AddTextureSRV("AlbedoTexture", marbleSRV);
	materials[counter]->AddTextureSRV("NormalTexture", marbleNormalSRV);
	materials[counter]->AddTextureSRV("RoughnessTexture", marbleRoughnessSRV);
	materials[counter]->AddTextureSRV("MetalnessTexture", (marbleMetalnessSRV != nullptr) ? marbleMetalnessSRV : fullNonMetalSRV);
	materials[counter]->AddSampler("BasicSampler", samplerState);
	materials[counter]->AddSampler("ClampSampler", clampState);
	counter++; // Increment counter to be in next Material's position
	
	// Metal Plates - possibly the only texture without gamma correction built in
	//materials.push_back(std::make_shared<Material>(vertexShader, pixelShader, XMFLOAT4(1.f, 1.f, 1.f, 1.f), 0.f));
	//materials[counter]->AddTextureSRV("AlbedoTexture", metalPlatesSRV);
	//materials[counter]->AddTextureSRV("NormalTexture", metalPlatesNormalSRV);
	//materials[counter]->AddTextureSRV("RoughnessTexture", metalPlatesRoughnessSRV);
	//materials[counter]->AddTextureSRV("MetalnessTexture", metalPlatesMetalnessSRV);
	//materials[counter]->AddSampler("BasicSampler", samplerState);
	//materials[counter]->AddSampler("ClampSampler", clampState);
	////materials[counter]->SetUVScale(.5f);
	//counter++;
	
	// Wood
	materials.push_back(std::make_shared<Material>(vertexShader, pixelShader, XMFLOAT4(1.f, 1.f, 1.f, 1.f), 0.f));
	materials[counter]->AddTextureSRV("AlbedoTexture", woodSRV);
	materials[counter]->AddTextureSRV("NormalTexture", woodNormalSRV);
	materials[counter]->AddTextureSRV("RoughnessTexture", woodRoughnessSRV);
	materials[counter]->AddTextureSRV("MetalnessTexture", (woodMetalnessSRV != nullptr) ? woodMetalnessSRV : fullNonMetalSRV);
	materials[counter]->AddSampler("BasicSampler", samplerState);
	materials[counter]->AddSampler("ClampSampler", clampState);
	counter++;
	
	// Metal
	materials.push_back(std::make_shared<Material>(vertexShader, pixelShader, XMFLOAT4(1.f, 1.f, 1.f, 1.f), 0.f));
	materials[counter]->AddTextureSRV("AlbedoTexture", metalSRV);
	materials[counter]->AddTextureSRV("NormalTexture", metalNormalSRV);
	materials[counter]->AddTextureSRV("RoughnessTexture", metalRoughnessSRV);
	materials[counter]->AddTextureSRV("MetalnessTexture", (metalMetalnessSRV != nullptr) ? metalMetalnessSRV :  fullNonMetalSRV);
	materials[counter]->AddSampler("BasicSampler", samplerState);
	materials[counter]->AddSampler("ClampSampler", clampState);
	counter++;

	// Cobblestone
	materials.push_back(std::make_shared<Material>(vertexShader, pixelShader, XMFLOAT4(1.f, 1.f, 1.f, 1.f), 0.f));
	materials[counter]->AddTextureSRV("AlbedoTexture", cobbleSRV);
	materials[counter]->AddTextureSRV("NormalTexture", cobbleNormalSRV);
	materials[counter]->AddTextureSRV("RoughnessTexture", cobbleRoughnessSRV);
	materials[counter]->AddTextureSRV("MetalnessTexture", cobbleMetalnessSRV);
	materials[counter]->AddSampler("BasicSampler", samplerState);
	materials[counter]->AddSampler("ClampSampler", clampState);
	materials[counter]->SetUVScale(.25f); // .5 looks good. .25 for Final Demo scene
	counter++;
	 
	// Bronze
	materials.push_back(std::make_shared<Material>(vertexShader, pixelShader, XMFLOAT4(1.f, 1.f, 1.f, 1.f), 0.f));
	materials[counter]->AddTextureSRV("AlbedoTexture", bronzeSRV);
	materials[counter]->AddTextureSRV("NormalTexture", bronzeNormalsSRV);
	materials[counter]->AddTextureSRV("RoughnessTexture", bronzeRoughnessSRV);
	materials[counter]->AddTextureSRV("MetalnessTexture", bronzeMetalnessSRV);
	materials[counter]->AddSampler("BasicSampler", samplerState);
	materials[counter]->AddSampler("ClampSampler", clampState);
	counter++;

	// Scratched Paint
	materials.push_back(std::make_shared<Material>(vertexShader, pixelShader, XMFLOAT4(1.f, 1.f, 1.f, 1.f), 0.f));
	materials[counter]->AddTextureSRV("AlbedoTexture", paintSRV);
	materials[counter]->AddTextureSRV("NormalTexture", paintNormalsSRV);
	materials[counter]->AddTextureSRV("RoughnessTexture", paintRoughnessSRV);
	materials[counter]->AddTextureSRV("MetalnessTexture", paintMetalnessSRV);
	materials[counter]->AddSampler("BasicSampler", samplerState);
	materials[counter]->AddSampler("ClampSampler", clampState);
	materials[counter]->SetUVScale(.75); // Set for Final Demo scene
	counter++;

	// Metallic Casing
	materials.push_back(std::make_shared<Material>(vertexShader, pixelShader, XMFLOAT4(1.f, 1.f, 1.f, 1.f), 0.f));
	materials[counter]->AddTextureSRV("AlbedoTexture", floorSRV);
	materials[counter]->AddTextureSRV("NormalTexture", floorNormalSRV);
	materials[counter]->AddTextureSRV("RoughnessTexture", floorRoughnessSRV);
	materials[counter]->AddTextureSRV("MetalnessTexture", floorMetalnessSRV);
	materials[counter]->AddSampler("BasicSampler", samplerState);
	materials[counter]->AddSampler("ClampSampler", clampState);
	materials[counter]->SetUVScale(.5f);
	counter++;

	// Pure Metal Materials
	for (int i = 0; i < 6; i++) {
		materials.push_back(std::make_shared<Material>(vertexShader, pixelShader, XMFLOAT4(1.f, 1.f, 1.f, 1.f), 1.f-((float)i / 5.f)));
		materials[counter]->AddTextureSRV("AlbedoTexture", fullMetalSRV);
		materials[counter]->AddTextureSRV("NormalTexture", defaultNormalSRV);
		materials[counter]->AddTextureSRV("RoughnessTexture", fullMetalSRV);
		materials[counter]->AddTextureSRV("MetalnessTexture", fullMetalSRV);
		materials[counter]->AddSampler("BasicSampler", samplerState);
		materials[counter]->AddSampler("ClampSampler", clampState);
		counter++;
	}

	// Pure Non-Metal Materials
	for (int i = 0; i < 6; i++) {
		materials.push_back(std::make_shared<Material>(vertexShader, pixelShader, XMFLOAT4(1.f, 1.f, 1.f, 1.f), 1.f - ((float)i / 5.f)));
		materials[counter]->AddTextureSRV("AlbedoTexture", fullMetalSRV);
		materials[counter]->AddTextureSRV("NormalTexture", defaultNormalSRV);
		materials[counter]->AddTextureSRV("RoughnessTexture", fullMetalSRV);
		materials[counter]->AddTextureSRV("MetalnessTexture", fullNonMetalSRV);
		materials[counter]->AddSampler("BasicSampler", samplerState);
		materials[counter]->AddSampler("ClampSampler", clampState);
		counter++;
	}
	// No need to increment counter

	// Procedural Pixel Shader
	materials.push_back(std::make_shared<Material>(vertexShader, customPixelShader));
}

// --------------------------------------------------------
// Creates some basic lights using the BasicLight struct
// --------------------------------------------------------
void Game::CreateLights()
{
	// Intense Directional Light facing from the Sun location in the (manually set) Sky
	BasicLight sunLight = {};
	sunLight.Type = LightType::Directional;
	sunLight.Direction = Vector3(0.f, .2f, -1.f); // Will be normalized in shader anyways
	sunLight.Color = Vector3(0.89f, 0.788f, 0.757f);
	sunLight.Intensity = 2.f;
	directionalLights.push_back(sunLight);
	
	// Other lights are removed to show off the IBL Diffuse lighting. Sunlight remains because it looks natural

	// Add a couple lights facing opposite the sun to remove extensive shadows
	//BasicLight backLight1 = {};
	//backLight1.Type = LightType::Directional;
	//backLight1.Direction = Vector3(.5f, .2f, 1.f); // Will be normalized in shader anyways
	//backLight1.Color = Vector3(0.89f, 0.788f, 0.757f);
	//backLight1.Intensity = 1.f;
	//directionalLights.push_back(backLight1);
	//
	//BasicLight backLight2 = {};
	//backLight2.Type = LightType::Directional;
	//backLight2.Direction = Vector3(-.5f, -.6f, 1.f); // Will be normalized in shader anyways
	//backLight2.Color = Vector3(0.89f, 0.788f, 0.757f);
	//backLight2.Intensity = 1.f;
	//directionalLights.push_back(backLight2);
	//
	//// White light to +X of the sphere
	//BasicLight pointLight1 = {};
	//pointLight1.Type = LightType::Directional;
	//pointLight1.Position = Vector3(2.f, 0.f, 0.f);
	//pointLight1.Range = 10.f;
	//pointLight1.Color = Vector3(1.f, 1.f, 1.f);
	//pointLight1.Intensity = 1.f;
	//pointLights.push_back(pointLight1);
	//std::shared_ptr<Entity> l1 = std::make_shared<Entity>(geometry[0], materials[materials.size() - 1]);
	//l1->GetTransform()->SetAbsolutePosition(pointLight1.Position);
	//l1->GetTransform()->SetAbsoluteScale(.1f, .1f, .1f);
	//entities.push_back(l1);
	//
	//// White light to -X of the sphere
	//BasicLight pointLight2 = {};
	//pointLight2.Type = LightType::Directional;
	//pointLight2.Position = Vector3(-2.f, 0.f, 0.f);
	//pointLight2.Range = 10.f;
	//pointLight2.Color = Vector3(1.f, 1.f, 1.f);
	//pointLight2.Intensity = 1.f;
	//pointLights.push_back(pointLight2);	std::shared_ptr<Entity> l2 = std::make_shared<Entity>(geometry[0], materials[materials.size() - 1]);
	//l2->GetTransform()->SetAbsolutePosition(pointLight2.Position);
	//l2->GetTransform()->SetAbsoluteScale(.1f, .1f, .1f);
	//entities.push_back(l2);
}

// ----------------------------------------------------------
// Create a texture representing the BRDF Lookup Table for IBL
// lighting. Since this is not object or material dependant, it
// can be created once and used for everything
// ----------------------------------------------------------
void Game::CreateIBLBRDFLookupTable()
{
	// Create Texture2D resource
	Microsoft::WRL::ComPtr<ID3D11Texture2D> brdfTableTex;
	D3D11_TEXTURE2D_DESC brdfTableDesc = {};
	brdfTableDesc.Height = 1024; // Hardcoded for now, but this is a reasonable size (used in Cascioli sample code)
	brdfTableDesc.Width = brdfTableDesc.Height;
	brdfTableDesc.MipLevels = 1;
	brdfTableDesc.ArraySize = 1;
	brdfTableDesc.Format = DXGI_FORMAT_R16G16_UNORM; // double-precision R,G - no B or A
	brdfTableDesc.SampleDesc.Count = 1;
	brdfTableDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;

	device->CreateTexture2D(&brdfTableDesc, nullptr, brdfTableTex.GetAddressOf()); // Make on GPU

	// Create SRV
	D3D11_SHADER_RESOURCE_VIEW_DESC tableSRVDesc = {};
	tableSRVDesc.Format = brdfTableDesc.Format;
	tableSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	tableSRVDesc.Texture2D.MipLevels = 1;
	tableSRVDesc.Texture2D.MostDetailedMip = 0;
	
	device->CreateShaderResourceView(brdfTableTex.Get(), &tableSRVDesc, iblBRDFLookupTexture.GetAddressOf());

	// Save previous render resources (viewport, depth buffer, render target)
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> backBufferRTV;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthBufferDSV;
	context->OMGetRenderTargets(1, backBufferRTV.GetAddressOf(), depthBufferDSV.GetAddressOf());
	UINT viewportCount = 1;
	D3D11_VIEWPORT cachedViewport;
	context->RSGetViewports(&viewportCount, &cachedViewport);

	// Turn off Vertex and Input Buffers in case they had data and set topology in case not yet set
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->IASetIndexBuffer(nullptr, DXGI_FORMAT_R32_UINT, 0);
	UINT stride = sizeof(Vertex); // Stride of everything else being drawn?
	UINT offset = 0;
	ID3D11Buffer* nullBuffer = nullptr; // Must pass address of nullptr to clear, not nullptr itself
	context->IASetVertexBuffers(0, 1, &nullBuffer, &stride, &offset);

	// Create Viewport for the texture
	D3D11_VIEWPORT textureViewport = {};
	textureViewport.MaxDepth = 1.f;
	textureViewport.Height = (FLOAT)brdfTableDesc.Height;
	textureViewport.Width = (FLOAT)brdfTableDesc.Width;

	context->RSSetViewports(viewportCount, &textureViewport);

	// Create RTV for texture
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> tableRTV;
	D3D11_RENDER_TARGET_VIEW_DESC tableRTVDesc = {};
	tableRTVDesc.Format = brdfTableDesc.Format;
	tableRTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	tableRTVDesc.Texture2D.MipSlice = 0;

	device->CreateRenderTargetView(brdfTableTex.Get(), &tableRTVDesc, tableRTV.GetAddressOf());
	float black[4] = {}; // Initialize to all zeroes
	context->ClearRenderTargetView(tableRTV.Get(), black);
	context->OMSetRenderTargets(1, tableRTV.GetAddressOf(), nullptr);

	// Set shaders and draw
	brdfLookupMapPixelShader->SetShader();
	fullscreenTriangleVertexShader->SetShader();
	context->Draw(3, 0);
	//context->Flush(); probably not necessary on this, but could be useful on the TexCube creations

	// Reset cached resources
	context->OMSetRenderTargets(1, backBufferRTV.GetAddressOf(), depthBufferDSV.Get());
	context->RSSetViewports(viewportCount, &cachedViewport);
}

// ----------------------------------------------------------
// Loads a Texture from a given filepath using the Game's
// Device and DeviceContext and the WICTextureLoader. Returns
// a ShaderResourceView and does not create an ID3D11Resource
//	- a_filePath: Relative filepath. Fixed using FixPath()
// ----------------------------------------------------------
Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Game::LoadTexture(std::wstring a_filePath)
{
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> textureResourceView;
	CreateWICTextureFromFile(
		device.Get(),
		context.Get(),
		FixPath(a_filePath).c_str(),
		nullptr,
		textureResourceView.GetAddressOf());
	return textureResourceView;
}

// --------------------------------------------------------
// Loads a Texture from a given filepath using the Game's
// Device and DeviceContext. Returns a ShaderResourceView 
// and does not create an ID3D11Resource. Function accepts a
// path to EITHER a .DDS file OR a folder containing 6 .png
// files named "front", "back", "left", "right", "up", and
// "down".
//	- a_filePath: Relative filepath. Fixed using FixPath()
//  - a_bIsDDS: Whether or not this is a single .DDS Texture
//	  Cube file. If it is not, code taken from Chris Cascioli's
//	  sample code is used to generate the Cube from 6 individual 
//	  textures
// --------------------------------------------------------
Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Game::LoadTextureCube(std::wstring a_filePath)
{
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> textureResourceView;
	// If this is a single file ending in the .dds extension. If not, it is assumed to be a folder containing 6 .pngs
	bool bIsDDS = (a_filePath.substr(a_filePath.size() - 4) == L".dds") ? true : false;

	// Single file, load using DDSTextureLoader
	if (bIsDDS) {
		CreateDDSTextureFromFile(
			device.Get(),
			context.Get(),
			FixPath(a_filePath).c_str(),
			nullptr,
			textureResourceView.GetAddressOf());
	}
	// Credit for the following code is to Chis Cascioli
	else {
		// Load the 6 textures into an array.
		// - We need references to the TEXTURES, not SHADER RESOURCE VIEWS!
		// - Explicitly NOT generating mipmaps, as we don't need them for the sky!
		// - Order matters here! +X, -X, +Y, -Y, +Z, -Z
		Microsoft::WRL::ComPtr<ID3D11Texture2D> textures[6] = {};
		CreateWICTextureFromFile(device.Get(), FixPath(a_filePath + L"/right.png").c_str(), (ID3D11Resource**)textures[0].GetAddressOf(), 0);
		CreateWICTextureFromFile(device.Get(), FixPath(a_filePath + L"/left.png").c_str(), (ID3D11Resource**)textures[1].GetAddressOf(), 0);
		CreateWICTextureFromFile(device.Get(), FixPath(a_filePath + L"/up.png").c_str(), (ID3D11Resource**)textures[2].GetAddressOf(), 0);
		CreateWICTextureFromFile(device.Get(), FixPath(a_filePath + L"/down.png").c_str(), (ID3D11Resource**)textures[3].GetAddressOf(), 0);
		CreateWICTextureFromFile(device.Get(), FixPath(a_filePath + L"/front.png").c_str(), (ID3D11Resource**)textures[4].GetAddressOf(), 0);
		CreateWICTextureFromFile(device.Get(), FixPath(a_filePath + L"/back.png").c_str(), (ID3D11Resource**)textures[5].GetAddressOf(), 0);

		// We'll assume all of the textures are the same color format and resolution,
		// so get the description of the first shader resource view
		D3D11_TEXTURE2D_DESC faceDesc = {};
		textures[0]->GetDesc(&faceDesc);

		// Describe the resource for the cube map, which is simply
		// a "texture 2d array" with the TEXTURECUBE flag set.
		// This is a special GPU resource format, NOT just a
		// C++ array of textures!!!
		D3D11_TEXTURE2D_DESC cubeDesc = {};
		cubeDesc.ArraySize = 6; // Cube map!
		cubeDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE; // We'll be using as a texture in a shader
		cubeDesc.CPUAccessFlags = 0; // No read back
		cubeDesc.Format = faceDesc.Format; // Match the loaded texture's color format
		cubeDesc.Width = faceDesc.Width; // Match the size
		cubeDesc.Height = faceDesc.Height; // Match the size
		cubeDesc.MipLevels = 1; // Only need 1
		cubeDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE; // A CUBE, not 6 separate textures
		cubeDesc.Usage = D3D11_USAGE_DEFAULT; // Standard usage
		cubeDesc.SampleDesc.Count = 1;
		cubeDesc.SampleDesc.Quality = 0;

		// Create the final texture resource to hold the cube map
		Microsoft::WRL::ComPtr<ID3D11Texture2D> cubeMapTexture;
		device->CreateTexture2D(&cubeDesc, 0, cubeMapTexture.GetAddressOf());

		// Loop through the individual face textures and copy them,
		// one at a time, to the cube map texure
		for (int i = 0; i < 6; i++) {
			// Calculate the subresource position to copy into
			unsigned int subresource = D3D11CalcSubresource(
				0, // Which mip (zero, since there's only one)
				i, // Which array element?
				1); // How many mip levels are in the texture?
			// Copy from one resource (texture) to another
			context->CopySubresourceRegion(
				cubeMapTexture.Get(), // Destination resource
				subresource, // Dest subresource index (one of the array elements)
				0, 0, 0, // XYZ location of copy
				textures[i].Get(), // Source resource
				0, // Source subresource index (we're assuming there's only one)
				0); // Source subresource "box" of data to copy (zero means the whole thing)
		}

		// At this point, all of the faces have been copied into the
		// cube map texture, so we can describe a shader resource view for it
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = cubeDesc.Format; // Same format as texture
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE; // Treat this as a cube!
		srvDesc.TextureCube.MipLevels = 1; // Only need access to 1 mip
		srvDesc.TextureCube.MostDetailedMip = 0; // Index of the first mip we want to see

		// Make SRV
		device->CreateShaderResourceView(cubeMapTexture.Get(), &srvDesc, textureResourceView.GetAddressOf());
	}

	return textureResourceView;
}

// --------------------------------------------------------
// Update the input to the UI and draw UI windows
// --------------------------------------------------------
void Game::UpdateUI(float deltaTime)
{
	// Get a reference to the input manager
	Input& input = Input::GetInstance();

	// Avoid tainting input
	input.SetKeyboardCapture(false);
	input.SetMouseCapture(false);

	// Send input to ImGUI
	ImGuiIO& io = ImGui::GetIO();
	io.DeltaTime = deltaTime;
	io.DisplaySize.x = (float)this->windowWidth;
	io.DisplaySize.y = (float)this->windowHeight;
	io.KeyCtrl = input.KeyDown(VK_CONTROL);
	io.KeyShift = input.KeyDown(VK_SHIFT);
	io.KeyAlt = input.KeyDown(VK_MENU);
	io.MousePos.x = (float)input.GetMouseX();
	io.MousePos.y = (float)input.GetMouseY();
	io.MouseDown[0] = input.MouseLeftDown();
	io.MouseDown[1] = input.MouseRightDown();
	io.MouseDown[2] = input.MouseMiddleDown();
	io.MouseWheel = input.GetMouseWheel();
	input.GetKeyArray(io.KeysDown, 256);

	// Reset Frame
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// Determine new capture status
	input.SetKeyboardCapture(io.WantCaptureKeyboard);
	input.SetMouseCapture(io.WantCaptureMouse);

	// Show windows
	//ImGui::ShowDemoWindow();
	UIStatsWindow();
	//UIEditorWindow(); Easier to comment out the whole window than to update it to ignore the now-missing point lights
}

// --------------------------------------------------------
// Generate a Stats window showing general application stats
//	- these are the same as those shown on the titlebar
// --------------------------------------------------------
void Game::UIStatsWindow()
{
	ImGui::Begin("Stats");

	// Version switch statement taken from DXCore::UpdateTitleBarStats()
	std::string directXVersion = "Direct3D Version: ";
	switch (dxFeatureLevel) {
	case D3D_FEATURE_LEVEL_11_1: directXVersion += "D3D 11.1"; break;
	case D3D_FEATURE_LEVEL_11_0: directXVersion += "D3D 11.0"; break;
	case D3D_FEATURE_LEVEL_10_1: directXVersion += "D3D 10.1"; break;
	case D3D_FEATURE_LEVEL_10_0: directXVersion += "D3D 10.0"; break;
	case D3D_FEATURE_LEVEL_9_3:  directXVersion += "D3D 9.3";  break;
	case D3D_FEATURE_LEVEL_9_2:  directXVersion += "D3D 9.2";  break;
	case D3D_FEATURE_LEVEL_9_1:  directXVersion += "D3D 9.1";  break;
	default:                     directXVersion += "D3D ???";  break;
	}

	ImGui::Text(directXVersion.c_str());
	ImGui::Text("Window Width: %d", this->windowWidth);
	ImGui::Text("Window Height: %d", this->windowHeight);
	ImGui::Text("Window Aspect Ratio: %.3f", (float)this->windowWidth / (float)this->windowHeight);
	ImGui::Text("FPS: %.3f", ImGui::GetIO().Framerate);
	ImGui::Text("Frame Time (MS): %.3f", ImGui::GetIO().DeltaTime * 1000);

	ImGui::End();
}

// --------------------------------------------------------
// Generate a window from which the Camera's values can be
// updated
//	- original goal was to get a dropdown to edit various
//	  Entities in the scene, but that caused assertion errors
// --------------------------------------------------------
void Game::UIEditorWindow()
{
	ImGui::Begin("Editor");

	// Dropdown to select entity to edit, based off of DearImGUI documentation
	// Not functioning
	//static std::shared_ptr<Entity> selectedEntity = entities[0];
	//static const char* selectedIndex = "1";
	//if (ImGui::BeginCombo("Entity Selection", selectedIndex)) {
	//	for (int i = 0; i < entities.size(); i++) {
	//		bool bIsSelected = (entities[i] == selectedEntity);
	//		if (ImGui::Selectable(std::to_string(i + 1).c_str(), bIsSelected)) {
	//			selectedEntity = entities[i];
	//			selectedIndex = std::to_string(i + 1).c_str();
	//		}
	//		if (bIsSelected) {
	//			ImGui::SetItemDefaultFocus();
	//		}
	//	}
	//	ImGui::EndCombo();
	//}

	// Edit Camera's values
	float fov = camera->GetFieldOfView();
	if (ImGui::SliderFloat("Camera Field Of View", &fov, XM_PIDIV4, XM_PIDIV4 * 3)) {
		camera->SetFieldOfView(fov);
	}
	float moveSpeed = camera->GetMovementSpeed();
	if (ImGui::SliderFloat("Camera Movement Speed", &moveSpeed, 0.f, 10.f)) {
		camera->SetMovementSpeed(moveSpeed);
	}
	float rotSpeed = camera->GetLookAtSpeed();
	if (ImGui::SliderFloat("Camera Rotation Speed", &rotSpeed, 0.f, 10.f)) {
		camera->SetLookAtSpeed(rotSpeed);
	}

	// Edit Point Light positions
	float pointLight1Pos[3] = { pointLights[0].Position.x, pointLights[0].Position.y, pointLights[0].Position.z };
	if (ImGui::SliderFloat3("Point Light 1 Position", &pointLight1Pos[0], -10, 10)) {
		pointLights[0].Position = Vector3(pointLight1Pos[0], pointLight1Pos[1], pointLight1Pos[2]);
		entities[entities.size() - 2]->GetTransform()->SetAbsolutePosition(pointLights[0].Position);
	}
	float pointLight2Pos[3] = { pointLights[1].Position.x, pointLights[1].Position.y, pointLights[1].Position.z };
	if (ImGui::SliderFloat3("Point Light 2 Position", &pointLight2Pos[0], -10, 10)) {
		pointLights[1].Position = Vector3(pointLight2Pos[0], pointLight2Pos[1], pointLight2Pos[2]);
		entities[entities.size() - 1]->GetTransform()->SetAbsolutePosition(pointLights[1].Position);
	}

	ImGui::End();
}

// --------------------------------------------------------
// Handle resizing to match the new window size.
//  - DXCore needs to resize the back buffer
//  - Eventually, we'll want to update our 3D camera
// --------------------------------------------------------
void Game::OnResize()
{
	// Prepare Renderer for Resize
	m_renderer->PreResize();

	// Handle base-level DX resize stuff
	DXCore::OnResize();

	// Set new Renderer info for new size
	m_renderer->PostResize(windowWidth, windowHeight, backBufferRTV, depthBufferDSV);

	// Resize Camera
	if (camera != nullptr) {
		camera->SetAspectRatio(XMINT2(this->windowWidth, this->windowHeight));
	}
}

// --------------------------------------------------------
// Update your game here - user input, move objects, AI, etc.
// --------------------------------------------------------
void Game::Update(float deltaTime, float totalTime)
{
	// Example input checking: Quit if the escape key is pressed
	if (Input::GetInstance().KeyDown(VK_ESCAPE))
		Quit();

	// Update UI immediately after checking to quit
	UpdateUI(deltaTime);

	// Update all entities with the deltaTime
	for (std::shared_ptr<Entity> entity : entities) {
		entity->Update(deltaTime);
	}

	// Update Camera
	if (camera != nullptr) {
		camera->Update(deltaTime);
	}
}

// --------------------------------------------------------
// Clear the screen, redraw everything, present to the user
// --------------------------------------------------------
void Game::Draw(float deltaTime, float totalTime)
{
	// Update Reflection Probes BEFORE clearing back buffer (not incredibly important, but they are a "pre-process" of sorts)
	//	- This results in a noticeable slowdown, even before convolution is done. ~45fps just rendering the scene cubemap
	//	  at 512x512 resolution per face
	//for (std::shared_ptr<ReflectionProbe> probe : reflectionProbes) {
	//	probe->Draw(device, context, entities, directionalLights, pointLights, sky, iblBRDFLookupTexture);
	//}

	//Microsoft::WRL::ComPtr<ID3D11RasterizerState> rsState;
	//D3D11_RASTERIZER_DESC rsDesc = {};
	//rsDesc.CullMode = D3D11_CULL_FRONT;
	//rsDesc.FrontCounterClockwise = FALSE;
	//rsDesc.FillMode = D3D11_FILL_SOLID;
	//device->CreateRasterizerState(&rsDesc, rsState.GetAddressOf());
	//context->RSSetState(rsState.Get());

	m_renderer->FrameStart();

	// Combine lights into one vector
	std::vector<BasicLight> allLights = std::vector<BasicLight>(directionalLights);
	allLights.insert(allLights.end(), pointLights.begin(), pointLights.end());

	m_renderer->Render(entities, allLights, camera, sky);

	m_renderer->PostProcess(camera);

	m_renderer->FrameEnd(vsync || !deviceSupportsTearing || isFullscreen);
}