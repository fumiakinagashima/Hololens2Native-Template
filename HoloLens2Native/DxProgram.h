#pragma once

#include "Cube.h"

struct swapchain_surfdata_t {
	ID3D11DepthStencilView* depth_view;
	ID3D11RenderTargetView* target_view;
};

struct swapchain_t {
	XrSwapchain handle;
	int32_t width;
	int32_t height;
	vector<XrSwapchainImageD3D11KHR> surface_images;
	vector<swapchain_surfdata_t>     surface_data;
};

struct input_state_t {
	XrActionSet actionSet;
	XrAction poseAction;
	XrAction selectAction;
	XrPath handSubactionPath[2];
	XrSpace handSpace[2];
	XrPosef handPose[2];
	XrBool32 renderHand[2];
	XrBool32 handSelect[2];
};

struct transform_buffer_t {
	XMFLOAT4X4 world;
	XMFLOAT4X4 viewproj;
};

class DxProgram
{
public:
	DxProgram();

	void Init(LUID& adapter_luid);
	void CompileShader();
	swapchain_surfdata_t MakeSurfaceData(XrBaseInStructure& swapchainImage);
	void Render(XrCompositionLayerProjectionView& layerView, ID3D11DepthStencilView* depthView, ID3D11RenderTargetView* targetView);
	void Terminate();

	ID3D11Device* Device();
	ID3D11DeviceContext* DeviceContext();

	// TODO change to abstruct objects.
	Cube* GetCube();
	void SetCube(Cube* cube);

private:
	ComPtr<ID3D11Device> m_device;
	ComPtr<ID3D11DeviceContext> m_context;
	ComPtr<ID3D11VertexShader> m_vertexShader;
	ComPtr<ID3D11PixelShader> m_pixelShader;
	ComPtr<ID3D11InputLayout> m_shaderLayout;
	ComPtr<ID3D11Buffer> m_vertexBuffer;
	ComPtr<ID3D11Buffer> m_indexBuffer;
	ComPtr<ID3D11Buffer> m_constantBuffer;

	Cube* m_cube;

};
