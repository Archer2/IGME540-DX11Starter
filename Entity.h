#pragma once

#include <memory>
#include <d3d11.h>
#include <wrl/client.h>

#include "Transform.h"
#include "Mesh.h"
#include "Camera.h"

//-------------------------------------------------------
// An Entity is a basic "renderable" object. The basic
// requirements are a Mesh of some kind and a Transform.
// Most other scene objects for rendering should in some
// way inherit from Entity.
//	- "renderable" because a Mesh can still be directly used, etc.
//-------------------------------------------------------
class Entity
{
public:
	// Construction/Destruction
	Entity(std::shared_ptr<Mesh> a_mesh);
	~Entity();

	// Core Functions
	void Update(float deltaTime);
	void Draw(Microsoft::WRL::ComPtr<ID3D11DeviceContext> a_d3dContext, Microsoft::WRL::ComPtr<ID3D11Buffer> a_vsConstantBuffer, std::shared_ptr<Camera> a_mainCamera);

	// Setters (The internal Mesh is not intended to be reset at this time)
	void SetTransform(Transform a_newTransform);

	// Getters for internal data
	Transform* GetTransform();
	std::shared_ptr<Mesh> GetMesh();

protected:
	float m_timeSinceCreation; // Recorded as an "object lifetime"
	Transform m_transform;
	std::shared_ptr<Mesh> m_mesh;
};
