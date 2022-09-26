#include "Camera.h"
#include "Input.h"

using namespace DirectX;

Camera::Camera(Transform a_initialTransform, DirectX::XMFLOAT2 a_aspectRatio)
	: m_transform(a_initialTransform)
	, m_projectionType(ProjectionType::Perspective)
	, m_fieldOfView(XM_PI / 3.f)
	, m_aspectRatio(a_aspectRatio)
	, m_nearClipDistance(0.01f)
	, m_farClipDistance(1000.f)
	, m_movementSpeed(2.f)
	, m_lookAtSpeed(2.f)
{
	m_viewMatrix = CalculateViewMatrix();
	m_projectionMatrix = CalculateProjectionMatrix();
}

Camera::~Camera()
{
	// Empty since there is no Heap data
}

void Camera::Update(float deltaTime)
{
	UpdateInput(deltaTime); // Must happen before View Matrix is updated
	UpdateViewMatrix(); // Happens every frame, important
}

void Camera::UpdateViewMatrix()
{
	// Only update if the Transform has changed. Since the world matrix for this Transform is never
	// collected, the dirty functionality can be overridden like this
	if (m_transform.IsTransformDirty()) {
		m_viewMatrix = CalculateViewMatrix();
		m_transform.SetTransformDirty(false);
	}
}

void Camera::UpdateProjectionMatrix(float a_fieldOfView, DirectX::XMFLOAT2 a_aspectRatio, float a_nearClipDistance, float a_farClipDistance)
{
	m_fieldOfView = a_fieldOfView;
	m_aspectRatio = a_aspectRatio;
	m_nearClipDistance = a_nearClipDistance;
	m_farClipDistance = a_farClipDistance;

	m_projectionMatrix = CalculateProjectionMatrix();
}

void Camera::SetFieldOfView(float a_newFOV)
{
	UpdateProjectionMatrix(a_newFOV, m_aspectRatio, m_nearClipDistance, m_farClipDistance);
}

void Camera::SetAspectRatio(DirectX::XMFLOAT2 a_aspectRatio)
{
	UpdateProjectionMatrix(m_fieldOfView, a_aspectRatio, m_nearClipDistance, m_farClipDistance);
}

void Camera::SetNearClipDistance(float a_newDistance)
{
	UpdateProjectionMatrix(m_fieldOfView, m_aspectRatio, a_newDistance, m_farClipDistance);
}

void Camera::SetFarClipDistance(float a_newDistance)
{
	UpdateProjectionMatrix(m_fieldOfView, m_aspectRatio, m_nearClipDistance, a_newDistance);
}

void Camera::SetProjectionType(ProjectionType a_projectionType)
{
	// Does not utilize UpdateProjectionMatrix because that is ambiguous on the type of projection used (and no relevant values are being reset)
	m_projectionType = a_projectionType;
	m_projectionMatrix = CalculateProjectionMatrix();
}

DirectX::XMFLOAT4X4 Camera::GetViewMatrix()
{
	return m_viewMatrix;
}

DirectX::XMFLOAT4X4 Camera::GetProjectionMatrix()
{
	return m_projectionMatrix;
}

Transform* Camera::GetTransform()
{
	return &m_transform;
}

DirectX::XMFLOAT4X4 Camera::CalculateViewMatrix()
{
	XMFLOAT4X4 viewMatrix;
	Vector3 position = m_transform.GetPosition();
	Vector3 forward = m_transform.GetForward();
	XMStoreFloat4x4(&viewMatrix, XMMatrixLookToLH(XMLoadFloat3(&position), XMLoadFloat3(&forward), XMLoadFloat3(&Transform::WorldUpwardVector)));
	return viewMatrix;
}

DirectX::XMFLOAT4X4 Camera::CalculateProjectionMatrix()
{
	XMFLOAT4X4 projMatrix;
	(m_projectionType == ProjectionType::Perspective) ?
		XMStoreFloat4x4(&projMatrix, XMMatrixPerspectiveFovLH(m_fieldOfView, m_aspectRatio.x / m_aspectRatio.y, m_nearClipDistance, m_farClipDistance))
		:
		XMStoreFloat4x4(&projMatrix, XMMatrixOrthographicLH(m_aspectRatio.x, m_aspectRatio.y, m_nearClipDistance, m_farClipDistance));
	return projMatrix;
}

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
	XMStoreFloat3(&movementVector, XMVectorScale(XMVector3Normalize(XMLoadFloat3(&movementVector)), m_movementSpeed * deltaTime));
	m_transform.Move(movementVector);

	if (input.MouseRightDown()) {
		XMFLOAT2 rotation;
		rotation.x = input.GetMouseXDelta();
		rotation.y = input.GetMouseYDelta();
		XMStoreFloat2(&rotation, XMVectorScale(XMVector2Normalize(XMLoadFloat2(&rotation)), m_lookAtSpeed * deltaTime));
		m_transform.AddAbsoluteRotation(0.f, rotation.y, rotation.x);
	}
}