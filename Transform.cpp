#include "Transform.h"

using namespace DirectX;

/************************* Static Transforms *************************/

const Transform Transform::ZeroTransform = Transform();
const Vector3 Transform::ZeroVector3 = Vector3(0.f, 0.f, 0.f);
const Vector3 Transform::OneVector3 = Vector3(1.f, 1.f, 1.f);
const Vector4 Transform::ZeroVector4 = Vector4(0.f, 0.f, 0.f, 0.f);
const Vector4 Transform::OneVector4 = Vector4(1.f, 1.f, 1.f, 1.f);
const Quaternion Transform::IdentityQuaternion = Quaternion(0.f, 0.f, 0.f, 1.f);
const Matrix4 Transform::IdentityMatrix4 = Matrix4(
	1.f, 0.f, 0.f, 0.f, 
	0.f, 1.f, 0.f, 0.f, 
	0.f, 0.f, 1.f, 0.f, 
	0.f, 0.f, 0.f, 1.f);
const Vector3 Transform::WorldForwardVector = Vector3(0.f, 0.f, 1.f);
const Vector3 Transform::WorldRightwardVector = Vector3(1.f, 0.f, 0.f);
const Vector3 Transform::WorldUpwardVector = Vector3(0.f, 1.f, 0.f);

/************************* Public Transform Methods ***************************/

//-----------------------------------------------
// Default Transform constructor creates an Identity
// (or Zero) Transform
//-----------------------------------------------
Transform::Transform()
	: m_absolutePosition(ZeroVector3)
	, m_absoluteScale(OneVector3)
	, m_absoluteRotation(IdentityQuaternion)
	, m_forwardVector(WorldForwardVector)
	, m_rightVector(WorldRightwardVector)
	, m_upVector(WorldUpwardVector)
	, m_worldTransform(IdentityMatrix4)
	, m_worldTransformInverseTranspose(IdentityMatrix4)
	, m_bTransformDirty(false)
	, m_bDirectionsDirty(false)
{
	//XMStoreFloat4x4(&m_worldTransform, XMMatrixIdentity());
	//XMStoreFloat4x4(&m_worldTransformInverseTranspose, XMMatrixIdentity());
}

//-----------------------------------------------
// Destructor takes care of nulling Heap values,
// even if not deleting them
//-----------------------------------------------
Transform::~Transform()
{
	// Empty now since no members are on the heap
}

//-----------------------------------------------
// Completely resets the Position
//-----------------------------------------------
void Transform::SetAbsolutePosition(Vector3 a_newPosition)
{
	m_absolutePosition = a_newPosition;
	m_bTransformDirty = true;
}

//-----------------------------------------------
// Completely resets the Position
//-----------------------------------------------
void Transform::SetAbsolutePosition(float x, float y, float z)
{
	m_absolutePosition = XMFLOAT3(x, y, z);
	m_bTransformDirty = true;
}

//-----------------------------------------------
// Completely resets the Scale
//-----------------------------------------------
void Transform::SetAbsoluteScale(Vector3 a_newScale)
{
	m_absoluteScale = a_newScale;
	m_bTransformDirty = true;
}

//-----------------------------------------------
// Completely resets the Scale
//-----------------------------------------------
void Transform::SetAbsoluteScale(float x, float y, float z)
{
	m_absoluteScale = XMFLOAT3(x, y, z);
	m_bTransformDirty = true;
}

//-----------------------------------------------
// Completely resets the Rotation
//-----------------------------------------------
void Transform::SetAbsoluteRotation(Quaternion a_newRotation)
{
	m_absoluteRotation = a_newRotation;
	m_bTransformDirty = true;
	m_bDirectionsDirty = true;
}

//-----------------------------------------------
// Completely resets the Rotation
//-----------------------------------------------
void Transform::SetAbsoluteRotation(float roll, float pitch, float yaw)
{
	XMStoreFloat4(&m_absoluteRotation, XMQuaternionRotationRollPitchYaw(pitch, yaw, roll));
	m_bTransformDirty = true;
	m_bDirectionsDirty = true;
}

//-----------------------------------------------
// Sets the state of the Transform (has it changed or not)
//-----------------------------------------------
void Transform::SetTransformDirty(bool bIsDirty)
{
	m_bTransformDirty = bIsDirty;
}

//-----------------------------------------------
// Updates the Position by adding the new value
//-----------------------------------------------
void Transform::AddAbsolutePosition(Vector3 a_addPosition)
{
	// Better as a multi-statement call or chained call? Does it matter other than readability?
	//XMVECTOR position = XMLoadFloat3(&m_absolutePosition);
	//position = XMVectorAdd(position, XMLoadFloat3(&a_addPosition));
	//XMStoreFloat3(&m_absolutePosition, position);
	XMStoreFloat3(&m_absolutePosition, XMVectorAdd(XMLoadFloat3(&m_absolutePosition), XMLoadFloat3(&a_addPosition)));

	m_bTransformDirty = true;
}

//-----------------------------------------------
// Updates the Position by adding the new value
//-----------------------------------------------
void Transform::AddAbsolutePosition(XMVECTOR a_addPosition)
{
	XMStoreFloat3(&m_absolutePosition, XMVectorAdd(XMLoadFloat3(&m_absolutePosition), a_addPosition));
	m_bTransformDirty = true;
}

//-----------------------------------------------
// Updates the Position by adding the new value
//-----------------------------------------------
void Transform::AddAbsolutePosition(float x, float y, float z)
{
	// Is it still worth it to create the object, load it to an XMVECTOR, and Store? Does creation change that?
	m_absolutePosition.x += x;
	m_absolutePosition.y += y;
	m_absolutePosition.z += z;
	m_bTransformDirty = true;
}

//-----------------------------------------------
// Updates the Scale by adding the new value
//-----------------------------------------------
void Transform::AddAbsoluteScale(Vector3 a_addScale)
{
	XMStoreFloat3(&m_absoluteScale, XMVectorAdd(XMLoadFloat3(&m_absoluteScale), XMLoadFloat3(&a_addScale)));
	m_bTransformDirty = true;
}

//-----------------------------------------------
// Updates the Scale by adding the new value
//-----------------------------------------------
void Transform::AddAbsoluteScale(XMVECTOR a_addScale)
{
	XMStoreFloat3(&m_absoluteScale, XMVectorAdd(XMLoadFloat3(&m_absoluteScale), a_addScale));
	m_bTransformDirty = true;
}

//-----------------------------------------------
// Updates the Scale by adding the new value
//-----------------------------------------------
void Transform::AddAbsoluteScale(float x, float y, float z)
{
	// Is it still worth it to create the object, load it to an XMVECTOR, and Store? Does creation change that?
	m_absoluteScale.x += x;
	m_absoluteScale.y += y;
	m_absoluteScale.z += z;
	m_bTransformDirty = true;
}

//-----------------------------------------------
// Updates the Rotation by adding the new value
//-----------------------------------------------
void Transform::AddAbsoluteRotation(Quaternion a_addRotation)
{
	XMStoreFloat4(&m_absoluteRotation, XMQuaternionMultiply(XMLoadFloat4(&m_absoluteRotation), XMLoadFloat4(&a_addRotation)));
	m_bTransformDirty = true;
	m_bDirectionsDirty = true;
}

//-----------------------------------------------
// Updates the Rotation by adding the new value
//-----------------------------------------------
void Transform::AddAbsoluteRotation(XMVECTOR a_addRotationQuaternion)
{
	XMStoreFloat4(&m_absoluteRotation, XMQuaternionMultiply(XMLoadFloat4(&m_absoluteRotation), a_addRotationQuaternion));
	m_bTransformDirty = true;
	m_bDirectionsDirty = true;
}

//-----------------------------------------------
// Updates the Rotation by adding the new value
//-----------------------------------------------
void Transform::AddAbsoluteRotation(float roll, float pitch, float yaw)
{
	XMStoreFloat4(&m_absoluteRotation, XMQuaternionMultiply(XMLoadFloat4(&m_absoluteRotation), XMQuaternionRotationRollPitchYaw(pitch, yaw, roll)));
	m_bTransformDirty = true;
	m_bDirectionsDirty = true;
}

//-----------------------------------------------
// Moves the Transform by the given vector along
// its current Orientation
//-----------------------------------------------
void Transform::Move(Vector3 a_addMovement)
{
	XMVECTOR addMovement = XMVector3Rotate(XMLoadFloat3(&a_addMovement), XMLoadFloat4(&m_absoluteRotation));
	XMStoreFloat3(&m_absolutePosition, XMVectorAdd(XMLoadFloat3(&m_absolutePosition), addMovement));
	m_bTransformDirty = true;
}

//-----------------------------------------------
// Moves the Transform by the given vector along
// its current Orientation
//-----------------------------------------------
void Transform::Move(DirectX::XMVECTOR a_addMovement)
{
	a_addMovement = XMVector3Rotate(a_addMovement, XMLoadFloat4(&m_absoluteRotation));
	XMStoreFloat3(&m_absolutePosition, XMVectorAdd(XMLoadFloat3(&m_absolutePosition), a_addMovement));
	m_bTransformDirty = true;
}

//-----------------------------------------------
// Moves the Transform by the given values along
// its current Orientation
//-----------------------------------------------
void Transform::Move(float x, float y, float z)
{
	Vector3 addMovementVector(x, y, z);
	XMVECTOR addMovement = XMVector3Rotate(XMLoadFloat3(&addMovementVector), XMLoadFloat4(&m_absoluteRotation));
	XMStoreFloat3(&m_absolutePosition, XMVectorAdd(XMLoadFloat3(&m_absolutePosition), addMovement));
	m_bTransformDirty = true;
}

//-----------------------------------------------
// Retrieves the full matrix representing this
// Transform's total World Transform
//-----------------------------------------------
Matrix4 Transform::GetWorldTransformMatrix()
{
	UpdateMatrices();
	return m_worldTransform;
}

//-----------------------------------------------
// Retrieves the full matrix representing this
// Transform's total World Transform transposed and
// inverted
//	- Helps to correct non-uniform scaling for normal
//	  calculations
//-----------------------------------------------
Matrix4 Transform::GetWorldTransformMatrixInverseTranspose()
{
	UpdateMatrices();
	return m_worldTransformInverseTranspose;
}

//-----------------------------------------------
// Retrives the Translation component of this Transform
//-----------------------------------------------
Vector3 Transform::GetPosition()
{
	return m_absolutePosition;
}

//-----------------------------------------------
// Retrives the Translation component of this Transform
//-----------------------------------------------
Vector3 Transform::GetScale()
{
	return m_absoluteScale;
}

//-----------------------------------------------
// Retrives the Translation component of this Transform
//-----------------------------------------------
Quaternion Transform::GetRotation()
{
	return m_absoluteRotation;
}

Vector3 Transform::GetForward()
{
	UpdateVectors();
	return m_forwardVector;
}

Vector3 Transform::GetRightward()
{
	UpdateVectors();
	return m_rightVector;
}

Vector3 Transform::GetUpward()
{
	UpdateVectors();
	return m_upVector;
}

//-----------------------------------------------
// Checks if this Transform has changed in any way
//-----------------------------------------------
bool Transform::IsTransformDirty()
{
	return m_bTransformDirty;
}

/***************************** Protected Transform Methods *****************************/

//-----------------------------------------------
// If the transform has changed, calculate new
// World Transformation Matrix.
//	- Calculates position * rotation * scale, since
//	  transformations are applied in the reverse order
//	- inlined if possible for a slight performance boost
//	  (many other funcs here could be too)
//-----------------------------------------------
inline void Transform::UpdateMatrices()
{
	if (m_bTransformDirty)
	{
		XMMATRIX position = XMMatrixTranslation(m_absolutePosition.x, m_absolutePosition.y, m_absolutePosition.z);
		XMMATRIX scale = XMMatrixScaling(m_absoluteScale.x, m_absoluteScale.y, m_absoluteScale.z);
		XMMATRIX rotation = XMMatrixRotationQuaternion(XMLoadFloat4(&m_absoluteRotation));

		XMMATRIX worldTransform = XMMatrixMultiply(scale, XMMatrixMultiply(rotation, position));
		XMStoreFloat4x4(&m_worldTransform, worldTransform);
		XMStoreFloat4x4(&m_worldTransformInverseTranspose, XMMatrixInverse(nullptr, XMMatrixTranspose(worldTransform)));

		m_bTransformDirty = false;
	}
}

inline void Transform::UpdateVectors()
{
	if (m_bDirectionsDirty) 
	{
		XMStoreFloat3(&m_forwardVector, XMVector3Rotate(XMLoadFloat3(&WorldForwardVector), XMLoadFloat4(&m_absoluteRotation)));
		XMStoreFloat3(&m_rightVector, XMVector3Rotate(XMLoadFloat3(&WorldRightwardVector), XMLoadFloat4(&m_absoluteRotation)));
		XMStoreFloat3(&m_upVector, XMVector3Rotate(XMLoadFloat3(&WorldUpwardVector), XMLoadFloat4(&m_absoluteRotation)));

		m_bDirectionsDirty = false;
	}
}
