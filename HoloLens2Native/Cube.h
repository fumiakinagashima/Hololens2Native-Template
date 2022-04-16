#pragma once

class Cube
{
public:
	Cube();

	D3D11_SUBRESOURCE_DATA* VertexBufferData();
	D3D11_SUBRESOURCE_DATA* IndexBufferData();
	CD3D11_BUFFER_DESC* VertexBufferDesc();
	CD3D11_BUFFER_DESC* IndexBufferDesc();
	
	XMFLOAT4* Orientation();
	XMFLOAT3* Position();

	UINT CountIndex();
	void SetPose(XrPosef pose);

private:
	float m_vertex[48] = {
		-1,-1,-1, -1,-1,-1,
		 1,-1,-1,  1,-1,-1,
		 1, 1,-1,  1, 1,-1,
		-1, 1,-1, -1, 1,-1,
		-1,-1, 1, -1,-1, 1,
		 1,-1, 1,  1,-1, 1,
		 1, 1, 1,  1, 1, 1,
		-1, 1, 1, -1, 2, 1
	};

	uint16_t m_index[36] = {
		1,2,0, 2,3,0, 4,6,5, 7,6,4,
		6,2,1, 5,6,1, 3,7,4, 0,3,4,
		4,5,1, 0,4,1, 2,7,3, 2,6,7
	};

	D3D11_SUBRESOURCE_DATA m_vertexBufferData;
	D3D11_SUBRESOURCE_DATA m_indexBufferData;
	CD3D11_BUFFER_DESC m_vertexBufferDesc;
	CD3D11_BUFFER_DESC m_indexBufferDesc;
	XrPosef m_pose;
};

