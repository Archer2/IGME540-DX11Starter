#pragma once

#include "DXCore.h"
#include "Mesh.h"
#include "Entity.h"
#include "Camera.h"

#include <DirectXMath.h>
#include <wrl/client.h> // Used for ComPtr - a smart pointer for COM objects
#include <memory>

class Game 
	: public DXCore
{

public:
	Game(HINSTANCE hInstance);
	~Game();

	// Overridden setup and game loop methods, which
	// will be called automatically
	void Init();
	void OnResize();
	void Update(float deltaTime, float totalTime);
	void Draw(float deltaTime, float totalTime);

private:

	// Initialization helper methods - feel free to customize, combine, remove, etc.
	void LoadShaders(); 
	void CreateGeometry();
	void GenerateEntities();

	// Note the usage of ComPtr below
	//  - This is a smart pointer for objects that abide by the
	//     Component Object Model, which DirectX objects do
	//  - More info here: https://github.com/Microsoft/DirectXTK/wiki/ComPtr

	// Buffers to hold actual geometry data
	//Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
	//Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;
	
	// Simple Mesh storage
	std::vector<std::shared_ptr<Mesh>> geometry;
	std::vector<std::shared_ptr<Entity>> entities; // Shared Pointers for consistency, and because they probably shouldn't be stack vars

	// Camera
	std::shared_ptr<Camera> camera;
	
	// Shaders and shader-related constructs
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader;
	Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout;
	Microsoft::WRL::ComPtr<ID3D11Buffer> vsConstantBuffer;
};

