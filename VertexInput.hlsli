#ifndef _VERTEX_SHADER_COMMON_INPUT_
#define _VERTEX_SHADER_COMMON_INPUT_

// Struct representing a single vertex worth of data
// - This should match the vertex definition in our C++ code
// - By "match", I mean the size, order and number of members
// - The name of the struct itself is unimportant, but should be descriptive
// - Each variable must have a semantic, which defines its usage
struct VertexShaderInput
{
	// Data type
	//  |
	//  |   Name          Semantic
	//  |    |                |
	//  v    v                v
	float3 localPosition	: POSITION;     // XYZ position
	float3 normal			: NORMAL;		// XYZ normal direction
	float3 tangent			: TANGENT;		// Tangent direction
	float2 uv				: TEXCOORD;		// UV texture coordinate
};

#endif