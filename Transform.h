#pragma once

#include <DirectXMath.h>
#include <memory>

// I really hate typing out the all-caps versions even without the namespaces, so Aliases!
using Vector3 = DirectX::XMFLOAT3;
using Vector4 = DirectX::XMFLOAT4;
using Quaternion = DirectX::XMFLOAT4;
using Matrix4 = DirectX::XMFLOAT4X4;

//-------------------------------------------------------
// The Transform class represents a basic 3D transform.
// Contains absolute and (in future) relative transforms.
//	- Also defines several constant, static common transforms
//-------------------------------------------------------
class Transform
{
public:
	// Define common Transforms as class-contained constants
	const static Transform ZeroTransform;
	const static Vector3 ZeroVector3;
	const static Vector3 OneVector3;
	const static Vector4 ZeroVector4;
	const static Vector4 OneVector4;
	const static Quaternion IdentityQuaternion;
	const static Matrix4 IdentityMatrix4;

	// Construction/Destruction
	Transform();
	~Transform();

	// Setters that overwrite relevant existing Transform data
	// Overloaded for multiple possible use cases
	void SetPosition(Vector3 a_newPosition);
	void SetPosition(float x, float y, float z);
	void SetScale(Vector3 a_newScale);
	void SetScale(float x, float y, float z);
	void SetRotation(Quaternion a_newRotation);
	void SetRotation(float roll, float pitch, float yaw);
	void SetTransformDirty(bool bIsDirty);

	// Modify Setters that add to the existing Transform
	// Overloaded for multiple possible use cases
	void AddAbsolutePosition(Vector3 a_addPosition);
	void AddAbsolutePosition(DirectX::XMVECTOR a_addPosition);
	void AddAbsolutePosition(float x, float y, float z);
	void AddAbsoluteScale(Vector3 a_addScale);
	void AddAbsoluteScale(DirectX::XMVECTOR a_addScale);
	void AddAbsoluteScale(float x, float y, float z);
	void AddAbsoluteRotation(Quaternion a_addRotation);
	void AddAbsoluteRotation(DirectX::XMVECTOR a_addRotationQuaternion); // XMVECTOR as a Quaternion
	void AddAbsoluteRotation(float roll, float pitch, float yaw);

	// Getters for internal data
	Matrix4 GetWorldTransformMatrix();
	Matrix4 GetWorldTransformMatrixInverseTranspose();
	Vector3 GetPosition();
	Vector3 GetScale();
	Quaternion GetRotation();
	bool IsTransformDirty();

protected:
	Vector3 m_absolutePosition;
	Vector3 m_absoluteScale;
	Quaternion m_absoluteRotation; // Quaternion - use XM...Quaternion() functions

	Matrix4 m_worldTransform;
	Matrix4 m_worldTransformInverseTranspose;

	bool m_bTransformDirty;

	inline void UpdateMatrices(); // inline if possible to boost performance (not forced since the function is longer)
};

