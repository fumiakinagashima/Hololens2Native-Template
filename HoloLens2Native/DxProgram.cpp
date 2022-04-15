#include "pch.h"
#include "DxProgram.h"

DxProgram::DxProgram() :
	m_device(nullptr),
	m_context(nullptr),
	m_vertexShader(nullptr),
	m_pixelShader(nullptr),
	m_shaderLayout(nullptr),
	m_vertexBuffer(nullptr),
	m_indexBuffer(nullptr),
	m_constantBuffer(nullptr)
{}

void DxProgram::Init(LUID& adapter_luid)
{
	IDXGIAdapter1* adapter = nullptr;
	IDXGIAdapter1* curr_adapter = nullptr;
	IDXGIFactory1* dxgi_factory;
	DXGI_ADAPTER_DESC1 adapter_desc;

	CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)(&dxgi_factory));

	int curr = 0;
	while (dxgi_factory->EnumAdapters1(curr++, &curr_adapter) == S_OK) {
		curr_adapter->GetDesc1(&adapter_desc);

		if (memcmp(&adapter_desc.AdapterLuid, &adapter_luid, sizeof(&adapter_luid)) == 0) {
			adapter = curr_adapter;
			break;
		}
		curr_adapter->Release();
		curr_adapter = nullptr;
	}
	dxgi_factory->Release();

	D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };

	HRESULT hr = D3D11CreateDevice(
		adapter,
		D3D_DRIVER_TYPE_UNKNOWN,
		0,
		0,
		featureLevels,
		_countof(featureLevels),
		D3D11_SDK_VERSION,
		&m_device,
		nullptr,
		&m_context);

	adapter->Release();
}

swapchain_surfdata_t DxProgram::MakeSurfaceData(XrBaseInStructure& swapchainImage)
{
	swapchain_surfdata_t result = {};

	XrSwapchainImageD3D11KHR& swapchainImageD3D = (XrSwapchainImageD3D11KHR&)swapchainImage;
	D3D11_TEXTURE2D_DESC colorDesc;
	swapchainImageD3D.texture->GetDesc(&colorDesc);

	D3D11_RENDER_TARGET_VIEW_DESC targetDesc = {};
	targetDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	targetDesc.Format = (DXGI_FORMAT)DXGI_FORMAT_R8G8B8A8_UNORM;
	m_device->CreateRenderTargetView(swapchainImageD3D.texture, &targetDesc, &result.target_view);

	ID3D11Texture2D* depthTexture;
	D3D11_TEXTURE2D_DESC depthDesc = {};
	depthDesc.SampleDesc.Count = 1;
	depthDesc.MipLevels = 1;
	depthDesc.Width = colorDesc.Width;
	depthDesc.Height = colorDesc.Height;
	depthDesc.ArraySize = colorDesc.ArraySize;
	depthDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	depthDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL;
	m_device->CreateTexture2D(&depthDesc, nullptr, &depthTexture);

	D3D11_DEPTH_STENCIL_VIEW_DESC stencilDesc = {};
	stencilDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	stencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
	m_device->CreateDepthStencilView(depthTexture, &stencilDesc, &result.depth_view);

	depthTexture->Release();

	return result;
}

void DxProgram::CompileShader()
{
	ComPtr<ID3DBlob> vsBlob;
	ComPtr<ID3DBlob> psBlob;

	D3DReadFileToBlob(L"VertexShader.cso", &vsBlob);
	D3DReadFileToBlob(L"PixelShader.cso", &psBlob);

	m_device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_vertexShader);
	m_device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_pixelShader);

	D3D11_INPUT_ELEMENT_DESC vertDesc[] = {
		{ "SV_POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	m_device->CreateInputLayout(
		vertDesc,
		(UINT)_countof(vertDesc),
		vsBlob->GetBufferPointer(),
		vsBlob->GetBufferSize(),
		&m_shaderLayout);
	
	m_device->CreateBuffer(m_cube->VertexBufferDesc(), m_cube->VertexBufferData(), m_vertexBuffer.GetAddressOf());
	m_device->CreateBuffer(m_cube->IndexBufferDesc(), m_cube->IndexBufferData(), m_indexBuffer.GetAddressOf());

	CD3D11_BUFFER_DESC constBufferDesc(sizeof(transform_buffer_t), D3D11_BIND_CONSTANT_BUFFER);
	m_device->CreateBuffer(&constBufferDesc, nullptr, m_constantBuffer.GetAddressOf());

}

void DxProgram::Render(
	XrCompositionLayerProjectionView& layerView,
	ID3D11DepthStencilView* depthView,
	ID3D11RenderTargetView* targetView)
{
	XrRect2Di& rect = layerView.subImage.imageRect;
	D3D11_VIEWPORT viewport = CD3D11_VIEWPORT(
		(float)rect.offset.x,
		(float)rect.offset.y,
		(float)rect.extent.width,
		(float)rect.extent.height);

	m_context->RSSetViewports(1, &viewport);

	float clear[] = { 0, 0, 0, 1 };
	m_context->ClearRenderTargetView(targetView, clear);
	m_context->ClearDepthStencilView(depthView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	m_context->OMSetRenderTargets(1, &targetView, depthView);

	float clipNear = 0.05f;
	float clipFar = 100.0f;
	float left = clipNear * tanf(layerView.fov.angleLeft);
	float right = clipNear * tanf(layerView.fov.angleRight);
	float down = clipNear * tanf(layerView.fov.angleDown);
	float up = clipNear * tanf(layerView.fov.angleUp);

	XMMATRIX matProjection = XMMatrixPerspectiveOffCenterRH(left, right, down, up, clipNear, clipFar);
	XMMATRIX matView = XMMatrixInverse(nullptr, XMMatrixAffineTransformation(
		DirectX::g_XMOne,
		DirectX::g_XMZero,
		XMLoadFloat4((XMFLOAT4*)&layerView.pose.orientation),
		XMLoadFloat3((XMFLOAT3*)&layerView.pose.position)));

	m_context->VSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());
	m_context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
	m_context->PSSetShader(m_pixelShader.Get(), nullptr, 0);

	UINT strides[] = { sizeof(float) * 6 };
	UINT offsets[] = { 0 };
	m_context->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), strides, offsets);
	m_context->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
	m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_context->IASetInputLayout(m_shaderLayout.Get());

	transform_buffer_t transformBuffer;
	XMStoreFloat4x4(&transformBuffer.viewproj, XMMatrixTranspose(matView * matProjection));

	// Drwa own Objects.
	XMMATRIX matrixModel = XMMatrixAffineTransformation(
		DirectX::g_XMOne * 0.05f,
		DirectX::g_XMZero,
		XMLoadFloat4(m_cube->Orientation()),
		XMLoadFloat3(m_cube->Position()));

	XMStoreFloat4x4(&transformBuffer.world, XMMatrixTranspose(matrixModel));
	m_context->UpdateSubresource(m_constantBuffer.Get(), 0, nullptr, &transformBuffer, 0, 0);
	m_context->DrawIndexed(m_cube->CountIndex(), 0, 0);
}

void DxProgram::Terminate()
{
	m_context.Reset();
	m_device.Reset();
}

ID3D11Device* DxProgram::Device()
{ 
	return m_device.Get();
}

ID3D11DeviceContext* DxProgram::DeviceContext()
{
	return m_context.Get();
}

Cube* DxProgram::GetCube()
{
	return m_cube;
}

void DxProgram::SetCube(Cube* cube)
{
	m_cube = cube;
}