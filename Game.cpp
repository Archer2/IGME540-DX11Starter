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

	// Create Camera some units behind the origin
	Transform cameraTransform = Transform::ZeroTransform;
	cameraTransform.SetAbsolutePosition(0.f, 0.f, -5.f);
	camera = std::make_shared<Camera>(cameraTransform, XMINT2(this->windowWidth, this->windowHeight));

	// Tell the input assembler (IA) stage of the pipeline what kind of
	// geometric primitives (points, lines or triangles) we want to draw.  
	// Essentially: "What kind of shape should the GPU draw with our vertices?"
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
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
}



// --------------------------------------------------------
// Loads basic geometry files
// --------------------------------------------------------
void Game::LoadGeometry()
{
	// Load default files provided in A6
	geometry.push_back(std::make_shared<Mesh>(FixPath(L"../../assets/meshes/sphere.obj").c_str(), device, context));
	geometry.push_back(std::make_shared<Mesh>(FixPath(L"../../assets/meshes/cube.obj").c_str(), device, context));
	geometry.push_back(std::make_shared<Mesh>(FixPath(L"../../assets/meshes/cylinder.obj").c_str(), device, context));
	geometry.push_back(std::make_shared<Mesh>(FixPath(L"../../assets/meshes/helix.obj").c_str(), device, context));
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
	// Generate 5 entities with random meshes and transforms
	for (int i = 0; i < 5; i++) {
		std::shared_ptr<Mesh> desiredMesh = geometry[std::rand() % geometry.size()]; // Random geometry
		std::shared_ptr<Material> desiredMat = materials[std::rand() % materials.size()]; // Random material
		std::shared_ptr<Entity> entity = std::make_shared<Entity>(desiredMesh, desiredMat);
		
		// Generate a random transform in 2D (still using normalized coordinates)
		Transform* pEntityTransform = entity->GetTransform();
		pEntityTransform->SetAbsolutePosition(GenerateRandomFloat(-1.f, 1.f), GenerateRandomFloat(-1.f, 1.f), 0.f);
		//float singleScale = GenerateRandomFloat(.5f, 1.5f);
		//pEntityTransform->SetAbsoluteScale(singleScale, singleScale, 1.f);
		pEntityTransform->SetAbsoluteRotation(GenerateRandomFloat(0.f, 2.f * XM_PI), 0.f, 0.f);

		// Push Entity to Game storage
		entities.push_back(entity);
	}
}

// ----------------------------------------------------------
// Initializes some basic Materials
// ----------------------------------------------------------
void Game::CreateMaterials()
{
	// Basic Pixel and Vertex Shaders
	materials.push_back(std::make_shared<Material>(vertexShader, pixelShader, XMFLOAT4(1.f, 0.5f, 0.5f, 1.f))); // Red color tint
	materials.push_back(std::make_shared<Material>(vertexShader, pixelShader, XMFLOAT4(0.5f, 1.f, 0.5f, 1.f))); // Blue color tint
	materials.push_back(std::make_shared<Material>(vertexShader, pixelShader, XMFLOAT4(0.5f, 0.5f, 1.f, 1.f))); // Green color tint
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
	UIEditorWindow();
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

	ImGui::End();
}

// --------------------------------------------------------
// Handle resizing to match the new window size.
//  - DXCore needs to resize the back buffer
//  - Eventually, we'll want to update our 3D camera
// --------------------------------------------------------
void Game::OnResize()
{
	// Handle base-level DX resize stuff
	DXCore::OnResize();

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
	// Frame START
	// - These things should happen ONCE PER FRAME
	// - At the beginning of Game::Draw() before drawing *anything*
	{
		// Clear the back buffer (erases what's on the screen)
		const float bgColor[4] = { 0.4f, 0.6f, 0.75f, 1.0f }; // Cornflower Blue
		context->ClearRenderTargetView(backBufferRTV.Get(), bgColor);

		// Clear the depth buffer (resets per-pixel occlusion information)
		context->ClearDepthStencilView(depthBufferDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
	}

	// Draw all stored Entities
	for (std::shared_ptr<Entity> entity : entities) {
		entity->Draw(context, camera);
	}

	// Frame END
	// - These should happen exactly ONCE PER FRAME
	// - At the very end of the frame (after drawing *everything*)
	{
		// Draw ImGUI interface LAST
		ImGui::Render();
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		// Present the back buffer to the user
		//  - Puts the results of what we've drawn onto the window
		//  - Without this, the user never sees anything
		swapChain->Present(vsync ? 1 : 0, 0);

		// Must re-bind buffers after presenting, as they become unbound
		context->OMSetRenderTargets(1, backBufferRTV.GetAddressOf(), depthBufferDSV.Get());
	}
}