#include "Mesh.h"

template<class T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

Mesh::Mesh(Vertex* a_vertices, UINT a_vertexCount, UINT* a_indices, UINT a_indexCount, ComPtr<ID3D11Device> a_device, ComPtr<ID3D11DeviceContext> a_context)
	: m_vertexBuffer(nullptr)
	, m_indexBuffer(nullptr)
	, m_deviceContext(a_context)
	, m_indexCount(a_indexCount)
{
	// Create the Vertex Buffer
	D3D11_BUFFER_DESC vbd = {};
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(Vertex) * a_vertexCount;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	vbd.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA vertexData = {};
	vertexData.pSysMem = a_vertices;

	a_device->CreateBuffer(&vbd, &vertexData, m_vertexBuffer.GetAddressOf());

	// Create the Index Buffer
	D3D11_BUFFER_DESC ibd = {};
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(UINT) * a_indexCount;
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;
	ibd.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA indexData = {};
	indexData.pSysMem = a_indices;

	a_device->CreateBuffer(&ibd, &indexData, m_indexBuffer.GetAddressOf());
}

Mesh::Mesh(std::vector<Vertex> a_vertices, std::vector<UINT> a_indices, ComPtr<ID3D11Device> a_device, ComPtr<ID3D11DeviceContext> a_context)
	: m_indexCount(a_indices.size())
{

}

Mesh::~Mesh()
{
	// Empty because ComPtr smart pointers take care of Releasing D3D pointers
}

void Mesh::Draw()
{
	UINT stride = sizeof(Vertex);
	UINT offset = 0;

	// Set Buffers
	m_deviceContext->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);
	m_deviceContext->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

	// Draw
	m_deviceContext->DrawIndexed(m_indexCount, 0, 0);
}

ComPtr<ID3D11Buffer> Mesh::GetVertexBuffer()
{
	return m_vertexBuffer;
}

ComPtr<ID3D11Buffer> Mesh::GetIndexBuffer()
{
	return m_indexBuffer;
}

UINT Mesh::GetIndexCount()
{
	return m_indexCount;
}
