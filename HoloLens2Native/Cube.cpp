#include "pch.h"
#include "Cube.h"

Cube::Cube():
	m_vertexBufferDesc(sizeof(m_vertex), D3D11_BIND_VERTEX_BUFFER),
	m_indexBufferDesc(sizeof(m_index), D3D11_BIND_INDEX_BUFFER)
{
	m_vertexBufferData = { m_vertex };
	m_indexBufferData = { m_index };
	m_pose = { {0, 0, 0, 1}, { 0, 0, 2 } };
}

D3D11_SUBRESOURCE_DATA* Cube::VertexBufferData()
{
	return &m_vertexBufferData;
}

CD3D11_BUFFER_DESC* Cube::VertexBufferDesc()
{
	return &m_vertexBufferDesc;
}

D3D11_SUBRESOURCE_DATA* Cube::IndexBufferData()
{
	return &m_indexBufferData;
}

CD3D11_BUFFER_DESC* Cube::IndexBufferDesc()
{
	return &m_indexBufferDesc;
}

UINT Cube::CountIndex()
{
	return (UINT)_countof(m_index);
}

XMFLOAT4* Cube::Orientation()
{
	return (XMFLOAT4*)&m_pose.orientation;
}

XMFLOAT3* Cube::Position()
{
	return (XMFLOAT3*)&m_pose.position;
}