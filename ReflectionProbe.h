#pragma once

#include <d3d11.h>
#include <wrl/client.h>

#include "simpleshader/SimpleShader.h"
#include "Transform.h"
#include "Entity.h"
#include "Lights.h"
#include "Sky.h"

//-------------------------------------------------------
// A ReflectionProbe represents a spherical volume of
// space that has localized reflections rendered, beyond
// the general skybox reflection
//	- Because it contains no physical scene representation
//	  it is not an Entity
//	- A spherical volume can be represented easily and has
//	  simple distance tests to nearby objects. Also, a
//	  uniform distance to the probe in all directions is
//	  slightly more accurate than a rectangular volume
//	- This probe is real-time for interesting effects, but
//	  could be abstracted out to a base class or set up
//	  with a flag to toggle updating on/off
//-------------------------------------------------------
class ReflectionProbe
{
public:
	// No default constructor necessary
	ReflectionProbe(
		float a_radius,
		Vector3 a_position,
		std::shared_ptr<SimpleVertexShader> a_sceneVS,
		std::shared_ptr<SimpleVertexShader> a_refVS,
		std::shared_ptr<SimplePixelShader> a_scenePS,
		std::shared_ptr<SimplePixelShader> a_refPS,
		Microsoft::WRL::ComPtr<ID3D11Device> a_d3dDevice
	);
	//~ReflectionProbe(); // Not needed, since there is no heap data

	void Draw(
		Microsoft::WRL::ComPtr<ID3D11Device> a_device,
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> a_d3dContext,
		const std::vector<std::shared_ptr<Entity>>& a_entities,
		const std::vector<BasicLight>& a_directionalLights,
		const std::vector<BasicLight>& a_pointLights,
		const std::shared_ptr<Sky> a_sky,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> a_brdfLookUp
	);

	// Setters
	void SetRadius(float a_radius) { m_radius = a_radius; }
	void SetPosition(Vector3 a_position) { m_position = a_position; }

	// Getters - use shorthand in header since they are so simple
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetReflectionMap() { return m_reflectionMap; }
	float GetRadius() { return m_radius; }
	Vector3 GetPostiion() { return m_position; }

protected:
	// The core reflection map itself, created with dimensions equal to the
	// box formed by the radius. Anytime the radius is changed this must be
	// re-created
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_reflectionMap;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_sceneCubeMap; // Used for pre-rendering for the reflection map
	Microsoft::WRL::ComPtr<ID3D11Texture2D> m_reflectionMapTexture; // Stored to create Render Target Views
	Microsoft::WRL::ComPtr<ID3D11Texture2D> m_sceneMapTexture;
	float m_radius;
	//Transform m_transform; Full Transform not required - no Scale or Rotation
	Vector3 m_position;

	// Storing shader references is icky. An AssetManager will help with that (and in several places in the Sky class)
	std::shared_ptr<SimpleVertexShader> m_sceneVertexShader; // Unused or not necessary
	std::shared_ptr<SimpleVertexShader> m_reflectionVertexShader;
	std::shared_ptr<SimplePixelShader> m_scenePixelShader; // Unused or not necessary
	std::shared_ptr<SimplePixelShader> m_reflectionPixelShader;

	void BuildResources(Microsoft::WRL::ComPtr<ID3D11Device> a_d3dDevice);
	void RenderScene(
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> a_rtv,
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> a_d3dContext,
		const std::vector<std::shared_ptr<Entity>>& a_entities,
		const std::vector<BasicLight>& a_directionalLights,
		const std::vector<BasicLight>& a_pointLights,
		const std::shared_ptr<Sky> a_sky,
		std::shared_ptr<Camera> a_camera,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> a_brdfLookUp
	);
};
