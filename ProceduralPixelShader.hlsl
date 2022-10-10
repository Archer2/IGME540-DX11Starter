
// cbuffer for necessary information from the CPU
cbuffer PixelConstantData : register(b0)
{
	float c_time;		// Total time active
}

// Struct representing the data we expect to receive from earlier pipeline stages
// - Matches the struct in the singular VertexShader
struct VertexToPixel
{
	float4 screenPosition	: SV_POSITION;
	float3 normal			: NORMAL;
	float2 uv				: TEXCOORD;
};

// Procedural Pixel Shader generates a 3x3 basic grid of colors
// in RGB and CYM, repeated a given number of times across the
// UV values. This grid of colors is then modified with each
// component undergoing a separate, simpler sin wave manipulation
//	- The math to get the 3x3 grid was very difficult, since I
//	  wanted to do it without branching
//	- Once that was done getting it to repeat was simpler
//	- Some simple sin wave modifications make it look a little
//	  cooler
float4 main(VertexToPixel input) : SV_TARGET
{
	// Start by clamping the UV values to 0-1 (necessary for algorithm)
	// max(0) % 1 is more efficient than clamp() when dealing with values > 1
	input.uv.x = max(input.uv.x, 0) % 1;
	input.uv.y = max(input.uv.y, 0) % 1;
	
	// Expand UV to be 0-3 instead of 0-1 (for the 3x3 grid)
	// Repeat that expansion a given number of times
	float gridSize = 3.f;
	float iterations = 5.f; // This one could definitely be passed in as a constant, but that is not needed right now
	float u = input.uv.x * gridSize * iterations;
	float v = input.uv.y * gridSize * iterations;

	// Divide U axis into thirds r->g->b columns from left to right
	float ur = 1 - min(floor(u%gridSize), 1); // 0<u<1 = 1, otherwise = 0
	float ug = min(floor(u%gridSize), 1) - abs(1 - max(floor(u%gridSize), 1)); // 1<u<2 = 1, otherwise = 0
	float ub = abs(1 - max(floor(u%gridSize), 1)); // 2<u<3 = 1, otherwise = 0

	// Divide V axis into thirds r->g->b rows from top to bottom
	float vr = 1 - min(floor(v%gridSize), 1); // 0<v<1 = 1, otherwise = 0
	float vg = min(floor(v%gridSize), 1) - abs(1 - max(floor(v%gridSize), 1)); // 1<v<2 = 1, otherwise = 0
	float vb = abs(1 - max(floor(v%gridSize), 1)); // 2<v<3 = 1, otherwise = 0

	// Add the values together to create some funky results. Least-identifyable
	// pattern I can find while playing with it is one axis shifted slightly
	float3 color = float3(ur + vb, ug + vr, ub + vg);

	// Simple sin-based modifier for each channel. I just played around with modifiers until it looked cool
	float mr = sin(c_time);
	float mg = sin(1.75f * c_time + 1);
	float mb = sin(1.3f * c_time + 2);
	float3 modifier = float3(mr, mg, mb) * .3f + .5f; // Clamp to positive values, in a smaller range than 0-1 
	
	return float4(color * modifier, 1.0f); // convert to 4-component vector and return
}