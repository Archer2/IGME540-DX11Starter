#pragma once

#include <DirectXMath.h>

#include "Transform.h"

//-------------------------------------------------------
// The Camera class represents a basic 3D camera.
// Contains logic to generate View and Projection matrices.
//	- Currently builds its own small transform over a Transform
//	  class to avoid Roll issues
//-------------------------------------------------------
class Camera
{
public:
	//-------------------------------------------------------
	// Enum to define types of 3D projections
	//-------------------------------------------------------
	enum class ProjectionType {
		Perspective = 0,
		Orthographic = 1
	};

	// Camera construction and destruction
	Camera(Transform a_initialTransform, DirectX::XMINT2 a_aspectRatio);
	~Camera();

	// Updating, both every frame and conditionally for matrices
	void Update(float deltaTime);

	void UpdateViewMatrix();
	void UpdateProjectionMatrix(float a_fieldOfView, DirectX::XMINT2 a_aspectRatio, float a_nearClipDistance, float a_farClipDistance);

	// Setters for Camera fields
	void SetFieldOfView(float a_newFOV);
	void SetAspectRatio(DirectX::XMINT2 a_aspectRatio);
	void SetNearClipDistance(float a_newDistance);
	void SetFarClipDistance(float a_newDistance);
	void SetProjectionType(ProjectionType a_projectionType); // TODO: Make this also go through UpdateProjectionMatix()?

	void SetMovementSpeed(float speed);
	void SetLookAtSpeed(float speed);

	// Camera rotation methods to handle manual Euler rotations - This should go through Transform eventually
	void AddCameraRotation(Vector3 a_rotationPitchYawRoll);
	void AddCameraRotation(float pitch, float yaw, float roll);

	// Getters for Camera Fields
	float GetFieldOfView();
	DirectX::XMINT2 GetAspectRatio();
	float GetNearClipDistance();
	float GetFarClipDistance();
	float GetMovementSpeed();
	float GetLookAtSpeed();

	Matrix4 GetViewMatrix();
	Matrix4 GetProjectionMatrix();
	Transform* GetTransform();

protected:
	Transform m_transform;

	Matrix4 m_viewMatrix;
	Matrix4 m_projectionMatrix;
	
	ProjectionType m_projectionType;

	float m_fieldOfView; // Radian angle
	DirectX::XMINT2 m_aspectRatio;
	float m_nearClipDistance;
	float m_farClipDistance;
	
	Vector2 m_rotationPitchYaw; // Pitch - x, Yaw = y. Placeholder to use Euler rotations, ignoring roll, since Quaternions from Transform add phantom Roll over time

	float m_movementSpeed;
	float m_lookAtSpeed;

	Matrix4 CalculateViewMatrix();
	Matrix4 CalculateProjectionMatrix();
	void UpdateInput(float deltaTime);
};

