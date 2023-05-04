#pragma once

#include <wrl/client.h>
#include <d3d11.h>
#include <vector>
#include <memory>

// These should be replaced with a single Scene
#include "Entity.h"
#include "Camera.h"
#include "Sky.h"
#include "Lights.h"

//----------------------------------------------------
// Contains very basic implementation of a Renderer
// class to separate actual Render logic from Game logic.
//	- No actual rendering optimizations are performed
//	  at this time, but it is planned that this will
//	  eventually be implemented (along with robust
//	  Scene representation)
//		- Including culling, object sorting, etc.
//	- It would be cool to get this working on a separate
//	  heavy-duty thread than the main Update loop, but
//	  that is an experiment for later <----------------- TODO
//----------------------------------------------------
class Renderer
{
public:

	// Enum dictating possible Render Target output types. This is used for targets of render
	// passes invoked with a Draw Call, not compute passes. Used for compile-time naming and organizing
	// of multiple targets
	enum RenderTarget {
		RT_SCENE_COLOR = 0,
		RT_SCENE_AMBIENT,
		RT_SCENE_NORMAL,
		RT_SCENE_DEPTH,

		RT_COUNT // Always last, internal integer representation marks the number of RTs AT COMPILE TIME!
	};

	// Performs same function as RenderTarget enum, but is specifically used for post process 
	// targets for compute shaders. They have separate requirements for binding and UAVs instead of RTVs
	//	- Having this whole process for 2 textures is a bit ridiculous, but when that turns into 4
	//	  Views its a bit nicer. It is also robust enough to support future additions
	enum PostProcessTarget {
		PPT_PASS_ZERO = 0, // Generic for any post-process type. If only 1 target is needed for the pass, this one should be used
		PPT_PASS_ONE, // Generic, secondary target for multi-step post-processes. Each pass can swap between writing and reading from each texture

		PPT_COUNT // Always last, internal integer representation marks the number of RTs AT COMPILE TIME!
	};

	Renderer(
		Microsoft::WRL::ComPtr<ID3D11Device> a_device, // Should be stored in DXCore and retrieved as needed
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> a_context, // Likewise ^
		Microsoft::WRL::ComPtr<IDXGISwapChain> a_swapChain,
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> a_backBufferRTV,
		Microsoft::WRL::ComPtr<ID3D11DepthStencilView> a_depthBufferDSV,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> a_iblBRDFLookupTexture, // Should live in AssetManager
		std::shared_ptr<SimpleVertexShader> a_fullscreenVS, // Just pass it in, since Game already loads it
		unsigned int a_windowWidth,
		unsigned int a_windowHeight
	);

	void PreResize();
	void PostResize(unsigned int a_windowWidth, unsigned int a_windowHeight, 
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> a_backBufferRTV,
		Microsoft::WRL::ComPtr<ID3D11DepthStencilView> a_depthBufferDSV);

	void FrameStart();
	void FrameEnd(bool a_vsync);

	// Parameters here can all be replaced with a single Scene object
	void Render(
		const std::vector<std::shared_ptr<Entity>>& a_entities,
		const std::vector<BasicLight>& a_allLights,
		const std::shared_ptr<Camera>& a_camera,
		const std::shared_ptr<Sky>& a_sky
	);
	
	void PostProcess(std::shared_ptr<Camera> a_camera);

	void DisplayRenderTextures(std::vector<RenderTarget> a_rtIndices, std::vector<PostProcessTarget> a_pptIndices);

protected:
	// Some or all of these do not need duplicate references stored here. They should be
	// able to query DXCore for some basic information to prevent it changing in multiple
	// places (device, back buffer, context(?), window dimensions)
	//	- IBL Lookup Texture should exist in an AssetManager, but for now Game is in charge
	//	  of all shader and universal resource loading and storing
	Microsoft::WRL::ComPtr<ID3D11Device> m_device; 
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_context;
	Microsoft::WRL::ComPtr<IDXGISwapChain> m_swapChain;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_backBufferRTV;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_depthBufferDSV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_iblBRDFLookupTexture;
	unsigned int m_windowWidth;
	unsigned int m_windowHeight;

	// How to store RTVs and SRVs for the MRT render targets?
	//	- Could have a separate member for each one, but that's unscalable, inflexible, 
	//	  and annoying
	//	- Could have an array or list, but then how to tell which is which? Build it in one order and
	//	  just set RTVs and SRVs in that order and hope it matches? No - inflexible
	//		- Using gauranteed integer keys into an array (via enum at compile-time) allows indexing
	//		  and re-ordering at the caller's whim - any combination can be set for rendering or reading
	//			- Excellent idea Professor!
	//	- Could have a map to "name" the target, but that is still inflexible - does not support a
	//	  varying number of post-processes with different render output needs
	//		- RTVs must be set in the same order as the SRVs in the PP shader, and a name does not
	//		  help find that order
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_mrtRTVs[RenderTarget::RT_COUNT];
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_mrtSRVs[RenderTarget::RT_COUNT];

	// Its possible these should just stick as UAVs
	//	- UAVs must be able to be bound as read-only resources 
	//	- Likewise, mrtSRVs may be able to be replaced by UAVs. Research must be done to see if UAVs can be bound
	//	  to t registers. Only issue is that mrt results should NEVER be overwritten during a frame, and a UAV could
	//	  allow them to be bound accidentally as a writable texture
	//	- The SRVs must exist as-is to be bound to the final pixel shader for drawing to the back buffer. This could
	//	  be eliminated by having that as a compute pass or by manually passing UAVs to that PS (not supported by
	//	  SimpleShader, but still possible)
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> m_ppUAVs[PostProcessTarget::PPT_COUNT];
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_ppSRVs[PostProcessTarget::PPT_COUNT];

	// Shaders and resources required for specifically SSAO
	std::shared_ptr<SimpleVertexShader> m_postProcessVS;
	std::shared_ptr<SimpleComputeShader> m_ssaoCoreCS;
	std::shared_ptr<SimpleComputeShader> m_ssaoBlurCS;
	std::shared_ptr<SimplePixelShader> m_ssaoCombinePS; // Remains a pixel shader to Draw to Back Buffer
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_ssaoRandomOffsets;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> m_standardSampler;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> m_clampSampler;
	std::vector<Vector4> m_ssaoOffsets;
};

