// SharedStructs.h defines structs for GPU communications that are shared
// accross the program. This includes constant buffers, etc.

#pragma once

#include <DirectXMath.h>

// Struct defining the constant data used by the Vertex Shader in
// VertexShader.hlsl
//	- Must match exactly with the struct in that file - if either changes
//	  the other must be updated
struct VertexShaderConstantData
{
	DirectX::XMFLOAT4 c_tintColor;
	DirectX::XMFLOAT4X4 c_worldTransform;
};