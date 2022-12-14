#include "ShaderHelpers.hlsli"

// Uses full-screen triangle approach detailed in Post-Processing slides
// to generate Position and UV information for the rasterizer
VertexToPixelFullscreenTriangle main( uint id : SV_VERTEXID )
{
	VertexToPixelFullscreenTriangle output;

	// Calculate the UV (0,0 to 2,2) via the ID
	// x = 0, 2, 0, 2, etc.
	// y = 0, 0, 2, 2, etc.
	output.UV = float2((id << 1) & 2, id & 2);

	// Convert uv to the (-1,1 to 3,-3) range for position
	output.Position = float4(output.UV, 0, 1);
	output.Position.x = output.Position.x * 2 - 1;
	output.Position.y = output.Position.y * -2 + 1;
	
	return output;
}