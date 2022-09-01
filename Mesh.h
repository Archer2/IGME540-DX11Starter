#pragma once

#include <d3d11.h>
#include <wrl/client.h>
#include <vector>

#include "Vertex.h"

class Mesh
{
public:
	Mesh(Vertex* a_vertices, UINT a_vertexCount, UINT* a_indices, UINT a_indexCount, Microsoft::WRL::ComPtr<ID3D11Device> a_device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> a_context);
	Mesh(std::vector<Vertex> a_vertices, std::vector<UINT> a_indices, Microsoft::WRL::ComPtr<ID3D11Device> a_device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> a_context);
	~Mesh();

	void Draw();

	Microsoft::WRL::ComPtr<ID3D11Buffer> GetVertexBuffer();
	Microsoft::WRL::ComPtr<ID3D11Buffer> GetIndexBuffer();
	UINT GetIndexCount();

private:
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_indexBuffer;

	Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_deviceContext;

	UINT m_indexCount;
};
