#include "pch.h"
#include "XrProgram.h"

XrProgram::XrProgram() :
	m_instance(nullptr),
	m_session(nullptr),
	m_sessionState(XR_SESSION_STATE_UNKNOWN),
	m_space(nullptr),
	m_systemId(XR_NULL_SYSTEM_ID),
	ext_xrGetD3D11GraphicsRequirementsKHR(nullptr)
{}

vector<const char*> XrProgram::SetExtensions()
{
	vector<const char*> use_extensions;
	uint32_t ext_count = 0;
	xrEnumerateInstanceExtensionProperties(nullptr, 0, &ext_count, nullptr);
	vector<XrExtensionProperties> xr_exts(ext_count, { XR_TYPE_EXTENSION_PROPERTIES });
	xrEnumerateInstanceExtensionProperties(nullptr, ext_count, &ext_count, xr_exts.data());

	for (size_t i = 0; i < xr_exts.size(); i++)
	{
		if (strcmp(XR_KHR_D3D11_ENABLE_EXTENSION_NAME, xr_exts[i].extensionName) == 0)
		{
			use_extensions.push_back(XR_KHR_D3D11_ENABLE_EXTENSION_NAME);
			break;
		}
	}

	return use_extensions;
}

void XrProgram::CreateInstance(const char* appName, vector<const char*> use_extensions)
{
	XrInstanceCreateInfo createInfo = { XR_TYPE_INSTANCE_CREATE_INFO };
	createInfo.enabledExtensionCount = use_extensions.size();
	createInfo.enabledExtensionNames = use_extensions.data();
	createInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
	strcpy_s(createInfo.applicationInfo.applicationName, appName);
	xrCreateInstance(&createInfo, &m_instance);
}

void XrProgram::RequestFormFactor()
{
	XrSystemGetInfo systemInfo = { XR_TYPE_SYSTEM_GET_INFO };
	systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
	xrGetSystem(m_instance, &systemInfo, &m_systemId);
}

void XrProgram::CheckBlendMode()
{
	uint32_t blend_count = 0;
	xrEnumerateEnvironmentBlendModes(
		m_instance,
		m_systemId,
		c_configView,
		1,
		&blend_count,
		&m_blend);
}

LUID XrProgram::GetGraphicsRequirements()
{
	xrGetInstanceProcAddr(m_instance, "xrGetD3D11GraphicsRequirementsKHR", (PFN_xrVoidFunction*)(&ext_xrGetD3D11GraphicsRequirementsKHR));
	XrGraphicsRequirementsD3D11KHR requirement = { XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR };
	ext_xrGetD3D11GraphicsRequirementsKHR(m_instance, m_systemId, &requirement);
	return requirement.adapterLuid;
}

void XrProgram::CreateSession(ID3D11Device* device)
{
	XrGraphicsBindingD3D11KHR binding = { XR_TYPE_GRAPHICS_BINDING_D3D11_KHR };
	binding.device = device;
	XrSessionCreateInfo sessionInfo = { XR_TYPE_SESSION_CREATE_INFO };
	sessionInfo.next = &binding;
	sessionInfo.systemId = m_systemId;
	xrCreateSession(m_instance, &sessionInfo, &m_session);
}

void XrProgram::CreateSpace()
{
	XrReferenceSpaceCreateInfo ref_space = { XR_TYPE_REFERENCE_SPACE_CREATE_INFO };
	ref_space.poseInReferenceSpace = c_poseIdentity;
	ref_space.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
	xrCreateReferenceSpace(m_session, &ref_space, &m_space);
}

void XrProgram::CreateSwapchain(DxProgram* dx)
{
	uint32_t view_count = 0;
	xrEnumerateViewConfigurationViews(m_instance, m_systemId, c_configView, 0, &view_count, nullptr);
	m_configViews.resize(view_count, { XR_TYPE_VIEW_CONFIGURATION_VIEW });
	m_views.resize(view_count, { XR_TYPE_VIEW });
	xrEnumerateViewConfigurationViews(m_instance, m_systemId, c_configView, view_count, &view_count, m_configViews.data());
	for (uint32_t i = 0; i < view_count; i++)
	{
		XrViewConfigurationView& view = m_configViews[i];
		XrSwapchainCreateInfo    swapchain_info = { XR_TYPE_SWAPCHAIN_CREATE_INFO };
		XrSwapchain              handle;
		swapchain_info.arraySize = 1;
		swapchain_info.mipCount = 1;
		swapchain_info.faceCount = 1;
		swapchain_info.format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapchain_info.width = view.recommendedImageRectWidth;
		swapchain_info.height = view.recommendedImageRectHeight;
		swapchain_info.sampleCount = view.recommendedSwapchainSampleCount;
		swapchain_info.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
		xrCreateSwapchain(m_session, &swapchain_info, &handle);

		uint32_t surface_count = 0;
		xrEnumerateSwapchainImages(handle, 0, &surface_count, nullptr);

		swapchain_t swapchain = {};
		swapchain.width = swapchain_info.width;
		swapchain.height = swapchain_info.height;
		swapchain.handle = handle;
		swapchain.surface_images.resize(surface_count, { XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR });
		swapchain.surface_data.resize(surface_count);
		xrEnumerateSwapchainImages(swapchain.handle, surface_count, &surface_count, (XrSwapchainImageBaseHeader*)swapchain.surface_images.data());
		for (uint32_t i = 0; i < surface_count; i++)
		{
			swapchain.surface_data[i] = dx->MakeSurfaceData((XrBaseInStructure&)swapchain.surface_images[i]);
		}
		m_swapchains.push_back(swapchain);
	}
}

void XrProgram::MakeActions()
{
	// TODO: Impliment your actions.
}

bool XrProgram::PollEvent()
{
	bool result = true;
	XrEventDataBuffer eventBuffer = { XR_TYPE_EVENT_DATA_BUFFER };

	while (xrPollEvent(m_instance, &eventBuffer) == XR_SUCCESS)
	{
		switch (eventBuffer.type)
		{
		case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED:
		{
			XrEventDataSessionStateChanged* changed = (XrEventDataSessionStateChanged*)&eventBuffer;
			m_sessionState = changed->state;

			switch (m_sessionState)
			{
			case XR_SESSION_STATE_READY:
			{
				XrSessionBeginInfo beginInfo = { XR_TYPE_SESSION_BEGIN_INFO };
				beginInfo.primaryViewConfigurationType = c_configView;
				xrBeginSession(m_session, &beginInfo);
				m_running = true;
			} break;
			case XR_SESSION_STATE_STOPPING:
			{
				m_running = false;
				xrEndSession(m_session);
			} break;
			case XR_SESSION_STATE_EXITING:
			case XR_SESSION_STATE_LOSS_PENDING:
			{
				result = false;
			} break;
			}
		} break;
		case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING:
		{
			return false;
		} break;
		}
		eventBuffer = { XR_TYPE_EVENT_DATA_BUFFER };
	}

	return result;
}

void XrProgram::PollActions()
{
	// TODO: Impliment your poll actions logic.
}

void XrProgram::Render(DxProgram* dx)
{
	XrFrameState frameState = { XR_TYPE_FRAME_STATE };
	xrWaitFrame(m_session, nullptr, &frameState);
	xrBeginFrame(m_session, nullptr);

	XrCompositionLayerBaseHeader* layer = nullptr;
	XrCompositionLayerProjection layerProjection = { XR_TYPE_COMPOSITION_LAYER_PROJECTION };
	vector<XrCompositionLayerProjectionView> views;
	if (RenderLayer(frameState.predictedDisplayTime, views, layerProjection, dx))
	{
		layer = (XrCompositionLayerBaseHeader*)&layerProjection;
	}

	XrFrameEndInfo endInfo = { XR_TYPE_FRAME_END_INFO };
	endInfo.displayTime = frameState.predictedDisplayTime;
	endInfo.environmentBlendMode = m_blend;
	endInfo.layerCount = layer == nullptr ? 0 : 1;
	endInfo.layers = &layer;
	xrEndFrame(m_session, &endInfo);
}

void XrProgram::Terminate()
{
	for (auto swapchain : m_swapchains)
	{
		xrDestroySwapchain(swapchain.handle);
		for (auto surface_data : swapchain.surface_data)
		{
			surface_data.depth_view->Release();
			surface_data.target_view->Release();
		}
	}
	m_swapchains.clear();

	if (m_input.actionSet != XR_NULL_HANDLE)
	{
		if (m_input.handSpace[0] != XR_NULL_HANDLE) xrDestroySpace(m_input.handSpace[0]);
		if (m_input.handSpace[1] != XR_NULL_HANDLE) xrDestroySpace(m_input.handSpace[1]);
		xrDestroyActionSet(m_input.actionSet);
	}
	if (m_space != XR_NULL_HANDLE) xrDestroySpace(m_space);
	if (m_session != XR_NULL_HANDLE) xrDestroySession(m_session);
	if (m_instance != XR_NULL_HANDLE) xrDestroyInstance(m_instance);
}

bool XrProgram::RenderLayer(
	XrTime predictedTime,
	vector<XrCompositionLayerProjectionView>& projectionViews,
	XrCompositionLayerProjection& layer,
	DxProgram* dx)
{
	// Check Session State.
	if (m_sessionState != XR_SESSION_STATE_VISIBLE && m_sessionState != XR_SESSION_STATE_FOCUSED) return false;

	uint32_t viewCount = 0;
	XrViewState viewState = { XR_TYPE_VIEW_STATE };
	XrViewLocateInfo locateInfo = { XR_TYPE_VIEW_LOCATE_INFO };
	locateInfo.viewConfigurationType = c_configView;
	locateInfo.displayTime = predictedTime;
	locateInfo.space = m_space;
	xrLocateViews(m_session, &locateInfo, &viewState, (uint32_t)m_views.size(), &viewCount, m_views.data());
	projectionViews.resize(viewCount);

	for (uint32_t i = 0; i < viewCount; i++)
	{
		uint32_t imgId;
		XrSwapchainImageAcquireInfo acquireInfo = { XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO };
		xrAcquireSwapchainImage(m_swapchains[i].handle, &acquireInfo, &imgId);

		XrSwapchainImageWaitInfo waitInfo = { XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO };
		waitInfo.timeout = XR_INFINITE_DURATION;
		xrWaitSwapchainImage(m_swapchains[i].handle, &waitInfo);

		projectionViews[i] = { XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW };
		projectionViews[i].pose = m_views[i].pose;
		projectionViews[i].fov = m_views[i].fov;
		projectionViews[i].subImage.swapchain = m_swapchains[i].handle;
		projectionViews[i].subImage.imageRect.offset = { 0, 0 };
		projectionViews[i].subImage.imageRect.extent = { m_swapchains[i].width, m_swapchains[i].height };

		dx->Render(
			projectionViews[i],
			m_swapchains[i].surface_data[imgId].depth_view,
			m_swapchains[i].surface_data[imgId].target_view);

		XrSwapchainImageReleaseInfo releaseInfo = { XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };
		xrReleaseSwapchainImage(m_swapchains[i].handle, &releaseInfo);
	}
	layer.space = m_space;
	layer.viewCount = (uint32_t)projectionViews.size();
	layer.views = projectionViews.data();

	return true;
}

void XrProgram::PollPredicted(XrTime predicted_time)
{}

bool XrProgram::IsRunning()
{
	return m_running;
}

bool XrProgram::NeedSleep()
{
	return m_sessionState != XR_SESSION_STATE_VISIBLE && m_sessionState != XR_SESSION_STATE_FOCUSED;
}