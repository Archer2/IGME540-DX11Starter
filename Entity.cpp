#include "Entity.h"

using namespace DirectX;

//-----------------------------------------------
// Basic Constructor takes in a shared Mesh and
// Material and defaults to a Zero Transform
//-----------------------------------------------
Entity::Entity(std::shared_ptr<Mesh> a_mesh, std::shared_ptr<Material> a_material)
	: Entity(a_mesh, a_material, Transform::ZeroTransform)
{
}

//-----------------------------------------------
// Fully parameterized constructor
//-----------------------------------------------
Entity::Entity(std::shared_ptr<Mesh> a_mesh, std::shared_ptr<Material> a_material, Transform a_transform)
	: m_timeSinceCreation(0.f)
	, m_mesh(a_mesh)
	, m_material(a_material)
	, m_transform(a_transform)
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

	// Some dummy code - all Entities rotate a little bit around Y
	//m_transform.AddAbsoluteRotation(0.f, 0.f, XM_2PI * deltaTime / 16.f);
}

//-----------------------------------------------
// Handles DirectX calls for drawing this Entity
//	- In future this may migrate to a unified Renderer
//  - Most of the heavy lifting of passing data to
//    Shaders SHOULD be done through the Material,
//    but it is easier to access the data here
//-----------------------------------------------
void Entity::Draw(Microsoft::WRL::ComPtr<ID3D11DeviceContext> a_d3dContext, std::shared_ptr<Camera> a_mainCamera)
{
	std::shared_ptr<SimpleVertexShader> vertexShader = m_material->GetVertexShader();
	std::shared_ptr<SimplePixelShader> pixelShader = m_material->GetPixelShader();

	// Set material's shaders to Active on the GPU - do in Material? Call would have to be first
	vertexShader->SetShader();
	pixelShader->SetShader();

	// Set Shader variables - names must match those in the shader cbuffers
	vertexShader->SetMatrix4x4("c_worldTransform", m_transform.GetWorldTransformMatrix());
	vertexShader->SetMatrix4x4("c_worldInvTranspose", m_transform.GetWorldTransformMatrixInverseTranspose());
	vertexShader->SetMatrix4x4("c_viewMatrix", a_mainCamera->GetViewMatrix());
	vertexShader->SetMatrix4x4("c_projectionMatrix", a_mainCamera->GetProjectionMatrix());

	// Check for different Pixel Shader constant variables - TODO: This is not sustainable - cannot check every possible shader name
	if (pixelShader->HasVariable("c_cameraPosition"))
		pixelShader->SetFloat3("c_cameraPosition", a_mainCamera->GetTransform()->GetPosition());
	if (pixelShader->HasVariable("c_time"))
		pixelShader->SetFloat("c_time", m_timeSinceCreation);

	//vsData.c_tintColor = XMFLOAT4(.7f, .65f, 1.f, 1.f); // Nice blue highlight tint relic

	m_material->PrepareMaterial(); // Passing all data to the Material is still too cumbersome - pass Entity reference?

	// Copy data to Active Shaders - do in Material? Call would have to be last
	vertexShader->CopyAllBufferData();
	pixelShader->CopyAllBufferData();

	m_mesh->Draw(); // Draws the Mesh with set data
}

//-----------------------------------------------
// Takes in a new Transform and overwrites the existing
//-----------------------------------------------
void Entity::SetTransform(Transform a_newTransform)
{
	m_transform = a_newTransform;
}

//-----------------------------------------------
// Reset the Material used by this Entity
//-----------------------------------------------
void Entity::SetMaterial(std::shared_ptr<Material> a_material)
{
	m_material = a_material;
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

//-----------------------------------------------
// Returns a reference to the Material used by
// this Entity
//-----------------------------------------------
std::shared_ptr<Material> Entity::GetMaterial()
{
	return m_material;
}
