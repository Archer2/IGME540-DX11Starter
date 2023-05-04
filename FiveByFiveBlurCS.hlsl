/**
* Compute Pass to perform a 5x5 box blur on the screen. Uses a 5x5 blur to use direct pixel values,
* which loses some of the blending that can be obtained with sampling, but makes better use of the
* ability to directly access texel data. Additionally, ThreadID in this case does not need to be
* converted to UV. Checking for texture boundaries is also acceptable, since the extra cost should be
* far offset by not performing extra sample operations in most threadgroups
*/

// Any external data required for the box blur
cbuffer BlurData : register(b0)
{
	int2 c_windowDimensions;
};

// Source of blur and resulting image
Texture2D BlurTarget : register(t0);
RWTexture2D<unorm float4> BlurResult : register(u0);

// Compute function to run a simple 4x4 box blur across the screen
[numthreads(32, 32, 1)] // product of all dimensions must be <= 1024
void main( uint3 DTid : SV_DispatchThreadID )
{
	float4 blur = float4(0.f, 0.f, 0.f, 0.f);
	
	// 5x5 loop around this pixel
	for (int x = (int)DTid.x - 2; x <= (int)DTid.x + 2; x++) {
		if (x < 0 || x >= c_windowDimensions.x) continue; // Assumes 0-based id indexing, but skips when out of texture bounds

		for (int y = (int)DTid.y - 2; y <= (int)DTid.y + 2; y++) {
			// Skip when out of texture bounds
			if (y < 0 || y >= c_windowDimensions.y) continue;

			blur += BlurTarget[int2(x, y)]; // Blurs all channels. Greyscale images (like SSAO result) are not affected
		}
	}

	blur /= 16.f; // Average from 16 samples
	BlurResult[DTid.xy] = float4(blur.rgb, 1.f); // Cut alpha
}