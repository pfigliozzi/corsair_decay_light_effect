#include "CUESDK.h"
#include "CUELFX/CUELFX.h"
#include "Shared/LFX.h"
#include "CorsairLFX/CorsairLFX.h"
#include "CorsairLayers/CorsairLayers.h"

#include <Windows.h>

#include <iostream>
#include <vector>
#include <thread>
#include <tuple>
#include <conio.h>

using Clock = std::chrono::steady_clock;

const char* errorString(CorsairError error)
{
	switch (error) {
	case CE_Success:
		return "Success";
	case CE_ServerNotFound:
		return "Server not found";
	case CE_NoControl:
		return "No control";
	case CE_ProtocolHandshakeMissing:
		return "Protocol handshake missing";
	case CE_IncompatibleProtocol:
		return "Incompatible protocol";
	case CE_InvalidArguments:
		return "Invalid Arguments";
	default:
		return "Unknown error";
	}
}

std::vector<CorsairLedId> getLeds()
{
	std::vector<CorsairLedId> leds;
	for (int i = CLK_Escape; i <= CLK_Fn; ++i) {
		leds.push_back(static_cast<CorsairLedId>(i));
	}

	return leds;
}


int main(int argc, char *argv[])
{
	CorsairPerformProtocolHandshake();
	if (const auto error = CorsairGetLastError()) {
		std::cerr << "Protocol Handshake failed: " << errorString(error) << std::endl;
		system("pause");
		return -1;
	}
	
	if (!CorsairGetDeviceCount()) {
		std::cerr << "No devices found" << std::endl;
		system("pause");
		return -1;
	}

	auto ledPositions = CorsairGetLedPositions();
	if (!ledPositions) {
		std::cerr << "No led posiotns available" << std::endl;
		system("pause");
		return -1;
	}

	CUELFXSetLedPositions(ledPositions);
	
	CorsairLFXSetLedPositions(ledPositions);

	CorsairLayersInitialize(&CorsairSetLedsColorsAsync);

	auto leds = getLeds();

	std::cout << "Play first effect with black-color center\nPress any key to play next step...\n";
	auto base_effect = CUELFXCreateSolidColorEffect(leds.size(), leds.data(), { 255, 255, 0 });
	auto effect1Id = CorsairLayersPlayEffect(base_effect, 1);
	_getch();

	std::cout << "Playing effect...\nPress Escape to stop playback\n";
	system("pause");
	return 0;
}