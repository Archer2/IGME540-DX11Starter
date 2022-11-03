#include "ShaderHelpers.hlsli"
#include "VertexInput.hlsli"

// Struct representing the constant data used by the vertex shader
cbuffer VertexConstantData : register(b0)
{
	matrix c_worldTransform; // World Transform for the object
	matrix c_worldInvTranspose; // World Inverse Transpose Transfrom used for normal manipulation
	matrix c_viewMatrix; // View Matrix of the currently active Camera
	matrix c_projectionMatrix; // Projection Matrix of the currently active Camera
};

// --------------------------------------------------------
// The entry point (main method) for our vertex shader
// 
// - Input is exactly one vertex worth of data (defined by a struct)
// - Output is a single struct of data to pass down the pipeline
// - Named "main" because that's the default the shader compiler looks for
// --------------------------------------------------------
VertexToPixel main( VertexShaderInput input )
{
	// Set up output struct
	VertexToPixel output;

	// Here we're essentially passing the input position directly through to the next
	// stage (rasterizer), though it needs to be a 4-component vector now.  
	// - To be considered within the bounds of the screen, the X and Y components 
	//   must be between -1 and 1.  
	// - The Z component must be between 0 and 1.  
	// - Each of these components is then automatically divided by the W component
	matrix wvp = mul(c_projectionMatrix, mul(c_viewMatrix, c_worldTransform));
	output.screenPosition = mul(wvp, float4(input.localPosition, 1.0f));

	// Pass through the Normal, UV, and World Position
	output.normal = normalize(mul((float3x3)c_worldInvTranspose, input.normal)); // Must be re-normalized, so is this necessary?
	output.uv = input.uv;
	output.worldPosition = mul(c_worldTransform, float4(input.localPosition, 1.0f)).xyz;

	// Whatever we return will make its way through the pipeline to the
	// next programmable stage we're using (the pixel shader for now)
	return output;
}