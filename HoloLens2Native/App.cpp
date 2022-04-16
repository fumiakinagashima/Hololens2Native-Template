#include "pch.h"
#include "XrProgram.h"
#include "DxProgram.h"
#include "Cube.h"

int __stdcall wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int) {

	XrProgram xr = {};
	DxProgram dx = {};

	vector<const char*> use_extensions = xr.SetExtensions();
	xr.CreateInstance("OpenXL - HoloLens2", use_extensions);
	xr.RequestFormFactor();
	xr.CheckBlendMode();

	LUID adapter = xr.GetGraphicsRequirements();
	dx.Init(adapter);

	ID3D11Device* device = dx.Device();
	xr.CreateSession(device);

	xr.CreateSpace();
	xr.CreateSwapchain(&dx);

	xr.MakeActions();

	Cube cube{};
	dx.SetCube(&cube);
	dx.CompileShader();

	while (xr.PollEvent())
	{
		if (xr.IsRunning())
		{
			xr.PollActions();
			xr.UpdateObjectPose(&cube);
			xr.Render(&dx);
			if (xr.NeedSleep()) this_thread::sleep_for(chrono::milliseconds(250));
		}
	}

	xr.Terminate();
	dx.Terminate();

	return 0;
}
