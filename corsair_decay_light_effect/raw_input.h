#pragma once

#include <Windows.h>
#include <WinUser.h>

#include <iostream>
#include <vector>
#include <thread>
#include <tuple>
#include <conio.h>

#ifdef __cplusplus
extern "C"
{
#endif
	void registerRawInputDevice()
	{
		RAWINPUTDEVICE dev;
		dev.usUsagePage = 1;
		dev.usUsage = 6;
		dev.dwFlags = 0;
		dev.hwndTarget = NULL;

		RegisterRawInputDevices(&dev, 1, sizeof(dev));
	}

	void unregisterRawInputDevice()
	{
		RAWINPUTDEVICE dev;
		dev.usUsagePage = 1;
		dev.usUsage = 6;
		dev.dwFlags = RIDEV_REMOVE;
		dev.hwndTarget = NULL;
		RegisterRawInputDevices(&dev, 1, sizeof(dev));

		PostQuitMessage(0);
	}
	auto getDevices()
	{
		UINT nDevices = 0;
		GetRawInputDeviceList(NULL, &nDevices, sizeof(RAWINPUTDEVICELIST));

		// Got Any?
		if (nDevices < 1)
			return 0;// Exit

		// Allocate Memory For Device List
		PRAWINPUTDEVICELIST pRawInputDeviceList;
		pRawInputDeviceList = new RAWINPUTDEVICELIST[sizeof(RAWINPUTDEVICELIST) * nDevices];

		// Fill Device List Buffer
		int nResult;
		nResult = GetRawInputDeviceList(pRawInputDeviceList, &nDevices, sizeof(RAWINPUTDEVICELIST));
		
		// Got Device List?
		if (nResult < 0)
		{
			DWORD dw = GetLastError();
			// Clean Up
			delete[] pRawInputDeviceList;

			return 0;// Error
		}
		
		return nResult;
	}

#ifdef __cplusplus
} //exten "C"
#endif