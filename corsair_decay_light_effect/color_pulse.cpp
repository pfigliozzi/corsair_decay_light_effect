// progress.cpp : Defines the entry point for the console application.
//

#include "CUESDK.h"

#include <windows.h>
#include <iostream>
#include <thread>
#include <future>
#include <vector>

const char* toString(CorsairError error) 
{
	switch (error) {
	case CE_Success : 
		return "CE_Success";
	case CE_ServerNotFound:
		return "CE_ServerNotFound";
	case CE_NoControl:
		return "CE_NoControl";
	case CE_ProtocolHandshakeMissing:
		return "CE_ProtocolHandshakeMissing";
	case CE_IncompatibleProtocol:
		return "CE_IncompatibleProtocol";
	case CE_InvalidArguments:
		return "CE_InvalidArguments";
	default:
		return "unknown error";
	}
}

std::vector<CorsairLedColor> getAvailableKeys() 
{
	auto colorsVector = std::vector<CorsairLedColor>();
	for (auto deviceIndex = 0; deviceIndex < CorsairGetDeviceCount(); deviceIndex++) {
		if (auto deviceInfo = CorsairGetDeviceInfo(deviceIndex)) {
			switch (deviceInfo->type) {
			case CDT_Mouse: {
				auto numberOfKeys = deviceInfo->physicalLayout - CPL_Zones1 + 1;
				for (auto i = 0; i < numberOfKeys; i++) {
					auto ledId = static_cast<CorsairLedId>(CLM_1 + i);
					colorsVector.push_back(CorsairLedColor{ ledId, 0, 0, 0 });
				}
			} break;
			case CDT_Keyboard: {
				auto ledPositions = CorsairGetLedPositions();
				if (ledPositions) {
					for (auto i = 0; i < ledPositions->numberOfLed; i++) {
						auto ledId = ledPositions->pLedPosition[i].ledId;
						colorsVector.push_back(CorsairLedColor{ ledId, 0, 0, 0 });
					}
				}
			} break;
			case CDT_Headset: {
				colorsVector.push_back(CorsairLedColor{ CLH_LeftLogo, 0, 0, 0 });
				colorsVector.push_back(CorsairLedColor{ CLH_RightLogo, 0, 0, 0 });
			} break;
			}
		}
	}
	return colorsVector;
}

void performPulseEffect(std::vector<CorsairLedColor> &ledColorsVec)
{
	static auto waveDuration = 500;
	const auto timePerFrame = 25;
	for (auto x = .0; x < 2; x += static_cast<double>(timePerFrame) / waveDuration) {
		auto val = (1 - pow(x - 1, 2)) * 255;
		for (auto &ledColor : ledColorsVec) 
			ledColor.g = val;
		CorsairSetLedsColorsAsync(ledColorsVec.size(), ledColorsVec.data(), nullptr, nullptr);
		
		if (GetAsyncKeyState(VK_OEM_PLUS) && waveDuration > 100)
			waveDuration -= 100;
		if (GetAsyncKeyState(VK_OEM_MINUS) && waveDuration < 2000)
			waveDuration += 100;
		std::this_thread::sleep_for(std::chrono::milliseconds(timePerFrame));
	}
}

//int main()
//{
//	CorsairPerformProtocolHandshake();
//	if (const auto error = CorsairGetLastError()) {
//		std::cout << "Handshake failed: " << toString(error) << std::endl;
//		getchar();
//		return -1;
//	}
//	auto colorsVector = getAvailableKeys();
//	if (!colorsVector.empty()) {
//		std::cout << "Working... Use \"+\" or \"-\" to increase or decrease speed.\nPress Escape to close program..."; 
//		while (!GetAsyncKeyState(VK_ESCAPE)) {
//			performPulseEffect(colorsVector);
//			std::this_thread::sleep_for(std::chrono::milliseconds(25));
//		}
//	}
//	return 0;
//}
//
