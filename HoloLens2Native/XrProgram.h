#pragma once

#include "DxProgram.h"
#include "Cube.h"

class XrProgram
{

public:
	XrProgram();

	// Initialize functions
	vector<const char*> SetExtensions();
	void CreateInstance(const char* appName, vector<const char*> use_extensions);
	void RequestFormFactor();
	void CheckBlendMode();
	LUID GetGraphicsRequirements();
	void CreateSession(ID3D11Device* device);
	void CreateSpace();
	void CreateSwapchain(DxProgram* dx);

	// Main loop functions
	void MakeActions();
	bool PollEvent();
	void PollActions();
	bool IsRunning();
	bool NeedSleep();
	void Render(DxProgram* dx);
	void UpdateObjectPose(Cube* cube);

	// Shutdown function
	void Terminate();

private:

	const XrViewConfigurationType c_configView = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
	const XrPosef c_poseIdentity = { {0,0,0,1}, {0,0,0} };

	XrInstance m_instance;
	XrSession m_session;
	XrSessionState m_sessionState;
	XrSpace m_space;
	XrSystemId m_systemId;
	input_state_t m_input;
	XrEnvironmentBlendMode m_blend;
	vector<XrView> m_views;
	vector<XrViewConfigurationView> m_configViews;
	vector<swapchain_t> m_swapchains;

	PFN_xrGetD3D11GraphicsRequirementsKHR ext_xrGetD3D11GraphicsRequirementsKHR;
	bool m_running = false;

	bool RenderLayer(
		XrTime predictedTime,
		vector<XrCompositionLayerProjectionView>& projectionViews,
		XrCompositionLayerProjection& layer,
		DxProgram* dx);
	void PollPredicted(XrTime predicted_time);

};