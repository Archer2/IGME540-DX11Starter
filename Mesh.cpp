#include "Mesh.h"

template<class T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

Mesh::Mesh(Vertex* a_vertices, UINT a_vertexCount, UINT* a_indices, UINT a_indexCount, ComPtr<ID3D11Device> a_device, ComPtr<ID3D11DeviceContext> a_context)
	: m_vertexBuffer(nullptr)
	, m_indexBuffer(nullptr)
	, m_deviceContext(a_context)
	, m_indexCount(a_indexCount)
{
	// Create a VERTEX BUFFER
	// - This holds the vertex data of triangles for a single object
	// - This buffer is created on the GPU, which is where the data needs to
	//    be if we want the GPU to act on it (as in: draw it to the screen)
	// First, we need to describe the buffer we want Direct3D to make on the GPU
	//  - Note that this variable is created on the stack since we only need it once
	//  - After the buffer is created, this description variable is unnecessary
	D3D11_BUFFER_DESC vbd = {};
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(Vertex) * a_vertexCount;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0; // Do not include ability to access data from CPU
	vbd.MiscFlags = 0;
	vbd.StructureByteStride = 0;

	// Create the proper struct to hold the initial vertex data
	// - This is how we initially fill the buffer with data
	// - Essentially, we're specifying a pointer to the data to copy
	D3D11_SUBRESOURCE_DATA vertexData = {};
	vertexData.pSysMem = a_vertices; // Pointer to CPU memory to copy

	// Actually create the buffer on the GPU with the initial data
	// - Once we do this, we'll NEVER CHANGE DATA IN THE BUFFER AGAIN
	a_device->CreateBuffer(&vbd, &vertexData, m_vertexBuffer.GetAddressOf());

	// Create an INDEX BUFFER
	// - This holds indices to elements in the vertex buffer
	// - This is most useful when vertices are shared among neighboring triangles
	// - This buffer is created on the GPU, which is where the data needs to
	//    be if we want the GPU to act on it (as in: draw it to the screen)
	// Describe the buffer, as we did above, with two major differences
	//  - Byte Width (3 unsigned integers vs. 3 whole vertices)
	//  - Bind Flag (used as an index buffer instead of a vertex buffer) 
	D3D11_BUFFER_DESC ibd = {};
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(UINT) * a_indexCount;
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;
	ibd.StructureByteStride = 0;

	// Specify the initial data for this buffer, similar to above
	D3D11_SUBRESOURCE_DATA indexData = {};
	indexData.pSysMem = a_indices;

	// Actually create the buffer with the initial data
	// - Once we do this, we'll NEVER CHANGE THE BUFFER AGAIN
	a_device->CreateBuffer(&ibd, &indexData, m_indexBuffer.GetAddressOf());
}

Mesh::Mesh(std::vector<Vertex> a_vertices, std::vector<UINT> a_indices, ComPtr<ID3D11Device> a_device, ComPtr<ID3D11DeviceContext> a_context)
	: Mesh(&a_vertices[0], (UINT)a_vertices.size(), &a_indices[0], (UINT)a_indices.size(), a_device, a_context)
{
	// Constructor to take std::vectors of data instead of raw arrays, as those are more useful when managing large amounts of data (but do have overhead)
}

Mesh::~Mesh()
{
	// Empty because ComPtr smart pointers take care of Releasing D3D pointers
}

void Mesh::Draw()
{
	// DRAW geometry
	// - These steps are generally repeated for EACH object you draw
	// - Other Direct3D calls will also be necessary to do more complex things
	UINT stride = sizeof(Vertex);
	UINT offset = 0;

	// Set buffers in the input assembler (IA) stage
	//  - Do this ONCE PER OBJECT, since each object may have different geometry
	//  - For this demo, this step *could* simply be done once during Init()
	//  - However, this needs to be done between EACH DrawIndexed() call
	//     when drawing different geometry, so it's here as an example
	m_deviceContext->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);
	m_deviceContext->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

	// Tell Direct3D to draw
	//  - Begins the rendering pipeline on the GPU
	//  - Do this ONCE PER OBJECT you intend to draw
	//  - This will use all currently set Direct3D resources (shaders, buffers, etc)
	//  - DrawIndexed() uses the currently set INDEX BUFFER to look up corresponding
	//     vertices in the currently set VERTEX BUFFER
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
