#include "Camera.h"
#include "Input.h"

using namespace DirectX;

// ----------------------------------------------------------
// Most basic constructor takes a Transform and Aspect Ratio
// ----------------------------------------------------------
Camera::Camera(Transform a_initialTransform, DirectX::XMINT2 a_aspectRatio)
	: m_transform(a_initialTransform)
	, m_projectionType(ProjectionType::Perspective)
	, m_fieldOfView(XM_PI / 3.f)
	, m_aspectRatio(a_aspectRatio)
	, m_nearClipDistance(0.01f)
	, m_farClipDistance(1000.f)
	, m_rotationPitchYaw(0.f, 0.f)
	, m_movementSpeed(2.f)
	, m_lookAtSpeed(2.f)
{
	m_viewMatrix = CalculateViewMatrix();
	m_projectionMatrix = CalculateProjectionMatrix();
}

// ----------------------------------------------------------
// Destructor has no current required functionality
// ----------------------------------------------------------
Camera::~Camera()
{
	// Empty since there is no Heap data
}

// ----------------------------------------------------------
// Handles any per-frame updating for the camera
// ----------------------------------------------------------
void Camera::Update(float deltaTime)
{
	UpdateInput(deltaTime); // Must happen before View Matrix is updated
	UpdateViewMatrix(); // Happens every frame, important
}

// ----------------------------------------------------------
// Updates the View Matrix if necessary
// ----------------------------------------------------------
void Camera::UpdateViewMatrix()
{
	// Only update if the Transform has changed. Since the world matrix for this Transform is never
	// collected, the dirty functionality can be overridden like this
	if (m_transform.IsTransformDirty()) {
		m_viewMatrix = CalculateViewMatrix();
		m_transform.SetTransformDirty(false);
	}
}

// ----------------------------------------------------------
// Update the Projection Matrix, setting all input values
//	- Called by all individual value Set functions
// ----------------------------------------------------------
void Camera::UpdateProjectionMatrix(float a_fieldOfView, DirectX::XMINT2 a_aspectRatio, float a_nearClipDistance, float a_farClipDistance)
{
	m_fieldOfView = a_fieldOfView;
	m_aspectRatio = a_aspectRatio;
	m_nearClipDistance = a_nearClipDistance;
	m_farClipDistance = a_farClipDistance;

	m_projectionMatrix = CalculateProjectionMatrix();
}

// ----------------------------------------------------------
// Sets the FOV and updates the Projection
// ----------------------------------------------------------
void Camera::SetFieldOfView(float a_newFOV)
{
	UpdateProjectionMatrix(a_newFOV, m_aspectRatio, m_nearClipDistance, m_farClipDistance);
}

// ----------------------------------------------------------
// Sets the Aspect Ratio and updates the projection
// ----------------------------------------------------------
void Camera::SetAspectRatio(DirectX::XMINT2 a_aspectRatio)
{
	UpdateProjectionMatrix(m_fieldOfView, a_aspectRatio, m_nearClipDistance, m_farClipDistance);
}

// ----------------------------------------------------------
// Sets the Near Clip Distance and updates the projection
// ----------------------------------------------------------
void Camera::SetNearClipDistance(float a_newDistance)
{
	UpdateProjectionMatrix(m_fieldOfView, m_aspectRatio, a_newDistance, m_farClipDistance);
}

// ----------------------------------------------------------
// Sets the Far Clip Distance and updates the projection
// ----------------------------------------------------------
void Camera::SetFarClipDistance(float a_newDistance)
{
	UpdateProjectionMatrix(m_fieldOfView, m_aspectRatio, m_nearClipDistance, a_newDistance);
}

// ----------------------------------------------------------
// Sets the projection Type and updates the projection
// ----------------------------------------------------------
void Camera::SetProjectionType(ProjectionType a_projectionType)
{
	// Does not utilize UpdateProjectionMatrix because that is ambiguous on the type of projection used (and no relevant values are being reset)
	m_projectionType = a_projectionType;
	m_projectionMatrix = CalculateProjectionMatrix();
}

// ----------------------------------------------------------
// Sets the Camera's movement speed
// ----------------------------------------------------------
void Camera::SetMovementSpeed(float speed)
{
	m_movementSpeed = speed;
}

// ----------------------------------------------------------
// Sets the Camera's rotation speed
// ----------------------------------------------------------
void Camera::SetLookAtSpeed(float speed)
{
	m_lookAtSpeed = speed;
}

// ----------------------------------------------------------
// Adds Pitch and Yaw to the Camera's rotation in Euler angles
//	- Roll is ignored
// ----------------------------------------------------------
void Camera::AddCameraRotation(Vector3 a_rotationPitchYawRoll)
{
	m_rotationPitchYaw.x += a_rotationPitchYawRoll.x;
	if (m_rotationPitchYaw.x >= XM_PIDIV2) {
		m_rotationPitchYaw.x = XM_PIDIV2 - 0.001f;
	}
	else if (m_rotationPitchYaw.x <= -XM_PIDIV2) {
		m_rotationPitchYaw.x = -XM_PIDIV2 + 0.001f;
	}
	m_rotationPitchYaw.y += a_rotationPitchYawRoll.y;
	// No roll for a_rotationPitchYawRoll.z
}

// ----------------------------------------------------------
// Adds Pitch and Yaw to the Camera's rotation in Euler angles
//	- Roll is ignored
// ----------------------------------------------------------
void Camera::AddCameraRotation(float pitch, float yaw, float roll)
{
	AddCameraRotation(Vector3(pitch, yaw, roll));
}

// ----------------------------------------------------------
// Retrieves the Camera's Field of View
// ----------------------------------------------------------
float Camera::GetFieldOfView()
{
	return m_fieldOfView;
}

// ----------------------------------------------------------
// Retrieves the Camera's Aspect Ratio
// ----------------------------------------------------------
DirectX::XMINT2 Camera::GetAspectRatio()
{
	return m_aspectRatio;
}

// ----------------------------------------------------------
// Retrieves the Camera's Near Clip Distance
// ----------------------------------------------------------
float Camera::GetNearClipDistance()
{
	return m_nearClipDistance;
}

// ----------------------------------------------------------
// Retrieves the Camera's Far Clip Distance
// ----------------------------------------------------------
float Camera::GetFarClipDistance()
{
	return m_farClipDistance;
}

// ----------------------------------------------------------
// Retrieves the Camera's Movement Speed
// ----------------------------------------------------------
float Camera::GetMovementSpeed()
{
	return m_movementSpeed;
}

// ----------------------------------------------------------
// Retrieves the Camera's Rotational speed
// ----------------------------------------------------------
float Camera::GetLookAtSpeed()
{
	return m_lookAtSpeed;
}

// ----------------------------------------------------------
// Retrieves the Camera's View Matrix
// ----------------------------------------------------------
DirectX::XMFLOAT4X4 Camera::GetViewMatrix()
{
	return m_viewMatrix;
}

// ----------------------------------------------------------
// Retrieves the Camera's Projection Matrix
// ----------------------------------------------------------
DirectX::XMFLOAT4X4 Camera::GetProjectionMatrix()
{
	return m_projectionMatrix;
}

// ----------------------------------------------------------
// Retrieves the Camera's Transform as a pointer
// ----------------------------------------------------------
Transform* Camera::GetTransform()
{
	return &m_transform;
}

// ----------------------------------------------------------
// Calculates the Camera's View Matrix
// ----------------------------------------------------------
DirectX::XMFLOAT4X4 Camera::CalculateViewMatrix()
{
	XMFLOAT4X4 viewMatrix;
	Vector3 position = m_transform.GetPosition();
	XMVECTOR forward = XMVector3Rotate(XMLoadFloat3(&Transform::WorldForwardVector), XMQuaternionRotationRollPitchYaw(m_rotationPitchYaw.x, m_rotationPitchYaw.y, 0.f));
	XMStoreFloat4x4(&viewMatrix, XMMatrixLookToLH(XMLoadFloat3(&position), forward, XMLoadFloat3(&Transform::WorldUpwardVector)));
	return viewMatrix;
}

// ----------------------------------------------------------
// Calculates the Camera's Projection Matrix
// ----------------------------------------------------------
DirectX::XMFLOAT4X4 Camera::CalculateProjectionMatrix()
{
	XMFLOAT4X4 projMatrix;
	(m_projectionType == ProjectionType::Perspective) ?
		XMStoreFloat4x4(&projMatrix, XMMatrixPerspectiveFovLH(m_fieldOfView, (float)m_aspectRatio.x / (float)m_aspectRatio.y, m_nearClipDistance, m_farClipDistance))
		:
		XMStoreFloat4x4(&projMatrix, XMMatrixOrthographicLH((float)m_aspectRatio.x, (float)m_aspectRatio.y, m_nearClipDistance, m_farClipDistance));
	return projMatrix;
}

// ----------------------------------------------------------
// Handles updating of input relevant to the Camera
// ----------------------------------------------------------
void Camera::UpdateInput(float deltaTime)
{
	Input& input = Input::GetInstance();
	Vector3 movementVector = Transform::ZeroVector3;

	if (input.KeyDown('W')) {
		movementVector.z += 1.f;
	}
	if (input.KeyDown('S')) {
		movementVector.z -= 1.f;
	}
	if (input.KeyDown('D')) {
		movementVector.x += 1.f;
	}
	if (input.KeyDown('A')) {
		movementVector.x -= 1.f;
	}
	if (input.KeyDown(VK_SPACE)) {
		movementVector.y += 1.f;
	}
	if (input.KeyDown(VK_LSHIFT)) {
		movementVector.y -= 1.f;
	}
	
	// Normalize to length 1 (if not 0), then multiply to appropriate length
	// Prevents multi-directional movement not maintaining speed
	XMVECTOR movement = XMVectorScale(XMVector3Normalize(XMLoadFloat3(&movementVector)), m_movementSpeed * deltaTime);
	// Overwrite with custom camera Euler rotations ignoring Roll - Not always good to just ignore Roll
	m_transform.AddAbsolutePosition(XMVector3Rotate(movement, XMQuaternionRotationRollPitchYaw(m_rotationPitchYaw.x, m_rotationPitchYaw.y, 0.f)));
	//m_transform.Move(movement);

	if (input.MouseRightDown()) {
		XMFLOAT2 rotation; // Normalization operation will likely change Int values into Floating Point
		rotation.x = (float)input.GetMouseXDelta();
		rotation.y = (float)input.GetMouseYDelta();
		XMStoreFloat2(&rotation, XMVectorScale(XMVector2Normalize(XMLoadFloat2(&rotation)), m_lookAtSpeed * deltaTime));
		//m_transform.AddAbsoluteRotation(0.f, rotation.y, rotation.x);
		AddCameraRotation(rotation.y, rotation.x, 0.f);
	}
}
