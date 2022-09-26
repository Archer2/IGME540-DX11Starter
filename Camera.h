#pragma once

#include <DirectXMath.h>
#include "Transform.h"

class Camera
{
public:
	enum class ProjectionType {
		Perspective = 0,
		Orthographic = 1
	};

	Camera(Transform a_initialTransform, DirectX::XMINT2 a_aspectRatio);
	~Camera();

	void Update(float deltaTime);

	void UpdateViewMatrix();
	void UpdateProjectionMatrix(float a_fieldOfView, DirectX::XMINT2 a_aspectRatio, float a_nearClipDistance, float a_farClipDistance);

	void SetFieldOfView(float a_newFOV);
	void SetAspectRatio(DirectX::XMINT2 a_aspectRatio);
	void SetNearClipDistance(float a_newDistance);
	void SetFarClipDistance(float a_newDistance);
	void SetProjectionType(ProjectionType a_projectionType); // TODO: Make this also go through UpdateProjectionMatix()?

	DirectX::XMFLOAT4X4 GetViewMatrix();
	DirectX::XMFLOAT4X4 GetProjectionMatrix();
	Transform* GetTransform();

protected:
	Transform m_transform;

	DirectX::XMFLOAT4X4 m_viewMatrix;
	DirectX::XMFLOAT4X4 m_projectionMatrix;
	
	ProjectionType m_projectionType;

	float m_fieldOfView; // Radian angle
	DirectX::XMINT2 m_aspectRatio;
	float m_nearClipDistance;
	float m_farClipDistance;
	
	float m_movementSpeed;
	float m_lookAtSpeed;

	DirectX::XMFLOAT4X4 CalculateViewMatrix();
	DirectX::XMFLOAT4X4 CalculateProjectionMatrix();
	void UpdateInput(float deltaTime);
};

