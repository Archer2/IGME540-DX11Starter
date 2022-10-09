#pragma once

#include <d3d11.h>
#include <wrl/client.h>
#include <vector>

#include "Vertex.h"

/// <summary>
/// The Mesh class wraps drawing functionality (as well as Vertex and Index storage) into a self-contained data structure that
/// can be used to scale with many different types of geometry.
/// </summary>
class Mesh
{
public:
	Mesh(Vertex* a_vertices, UINT a_vertexCount, UINT* a_indices, UINT a_indexCount, Microsoft::WRL::ComPtr<ID3D11Device> a_device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> a_context);
	Mesh(std::vector<Vertex> a_vertices, std::vector<UINT> a_indices, Microsoft::WRL::ComPtr<ID3D11Device> a_device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> a_context);
	Mesh(const wchar_t* a_fileName, Microsoft::WRL::ComPtr<ID3D11Device> a_device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> a_context);
	~Mesh();

	void Draw();

	Microsoft::WRL::ComPtr<ID3D11Buffer> GetVertexBuffer();
	Microsoft::WRL::ComPtr<ID3D11Buffer> GetIndexBuffer();
	UINT GetIndexCount();

private:
	void CreateMesh(Vertex* a_vertices, UINT a_vertexCount, UINT* a_indices, UINT a_indexCount, Microsoft::WRL::ComPtr<ID3D11Device> a_device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> a_context);

	Microsoft::WRL::ComPtr<ID3D11Buffer> m_vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_indexBuffer;

	Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_deviceContext;

	UINT m_indexCount;
};
