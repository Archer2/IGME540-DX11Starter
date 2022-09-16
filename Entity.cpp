#include "Entity.h"
#include "SharedStructs.h"

using namespace DirectX;

//-----------------------------------------------
// Basic Constructor takes in a shared Mesh and
// defaults to a Zero Transform
//-----------------------------------------------
Entity::Entity(std::shared_ptr<Mesh> a_mesh)
	: m_timeSinceCreation(0.f)
	, m_mesh(a_mesh)
	, m_transform(Transform::ZeroTransform)
{
}

//-----------------------------------------------
// Clean the Heap of class data
//-----------------------------------------------
Entity::~Entity()
{
	// No heap to clean up yet
}

//-----------------------------------------------
// Handles updating of any game-related operations
//	- At a minimum updates time since creation
//-----------------------------------------------
void Entity::Update(float deltaTime)
{
	m_timeSinceCreation += deltaTime;

	// Some dummy code - all Entities rotate a little bit around Z
	m_transform.AddAbsoluteRotation(XM_2PI * deltaTime / 5.f, 0.f, 0.f);
}

//-----------------------------------------------
// Handles DirectX calls for drawing this Entity
//	- In future this may migrate to a unified Renderer
//-----------------------------------------------
void Entity::Draw(Microsoft::WRL::ComPtr<ID3D11DeviceContext> a_d3dContext, Microsoft::WRL::ComPtr<ID3D11Buffer> a_vsConstantBuffer)
{
	// Set Vertex Shader Const Data to correct values for this object
	VertexShaderConstantData vsData = {};
	vsData.c_tintColor = XMFLOAT4(.7f, .65f, 1.f, 1.f); // Each value is 0-1. This one gives a nice blue highlight
	vsData.c_worldTransform = m_transform.GetWorldTransformMatrix();

	// Set data to GPU
	D3D11_MAPPED_SUBRESOURCE mappedBuffer = {};
	a_d3dContext->Map(a_vsConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedBuffer);
	memcpy(mappedBuffer.pData, &vsData, sizeof(vsData));
	a_d3dContext->Unmap(a_vsConstantBuffer.Get(), 0);

	// Bind to Active
	a_d3dContext->VSSetConstantBuffers(0, 1, a_vsConstantBuffer.GetAddressOf());

	m_mesh->Draw(); // Draws the Mesh itself with set data
}

//-----------------------------------------------
// Takes in a new Transform and overwrites the existing
//-----------------------------------------------
void Entity::SetTransform(Transform a_newTransform)
{
	m_transform = a_newTransform;
}

//-----------------------------------------------
// Returns a pointer to the Transform so that it can
// be edited directly by the caller
//-----------------------------------------------
Transform* Entity::GetTransform()
{
	return &m_transform;
}

//-----------------------------------------------
// Returns a pointer to the internal Mesh
//-----------------------------------------------
std::shared_ptr<Mesh> Entity::GetMesh()
{
	return m_mesh;
}
