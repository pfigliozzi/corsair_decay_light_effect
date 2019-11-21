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

void playEffect(CorsairEffect *effect)
{
	auto startPoint = Clock::now();

	while (!GetAsyncKeyState(VK_ESCAPE)) {
		
		auto offset = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - startPoint).count();
		auto frame = CUELFXGetFrame(effect->effectId, static_cast<int>(offset));
		
		if (frame && frame->ledsColors) {
			auto res = CorsairSetLedsColors(frame->size, frame->ledsColors);
			CUELFXFreeFrame(frame);

			if (!res) {
				std::cerr << "Failed to set led colors: " << errorString(CorsairGetLastError()) << std::endl;
				return;
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(25));
	}
}

std::tuple<CorsairLightingEffectSpeed, CorsairLightingEffectCircularDirection> getEffectParams()
{
	CorsairLightingEffectSpeed speed = CLES_Medium;
	CorsairLightingEffectCircularDirection direction = CLECD_Clockwise;

	int inputParam = 0;
	std::cout << "Input effect speed (0 - Slow, 1 - Medium(default), 2 - Fast): ";
	std::cin >> inputParam;
	
	if (std::cin.fail()) {
		std::cin.clear();
		std::cin.ignore();
		inputParam = -1;
	}

	switch (inputParam) {
	case 0:
		speed = CLES_Slow;
		break;
	default:
		std::cout << "Use default speed\n";
	case 1:
		break;
	case 2:
		speed = CLES_Fast;
		break;
	}

	std::cout << "Input effect direction (0 - Clockwise(default), 1 - CounterClockwise): ";
	std::cin >> inputParam;

	if (std::cin.fail()) {
		std::cin.clear();
		std::cin.ignore();
		inputParam = -1;
	}

	switch (inputParam) {
	default:
		std::cout << "Use default direction\n";
	case 0:
		break;
	case 1:
		direction = CLECD_CounterClockwise;
		break;
	}

	return std::make_tuple(speed, direction);
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
	auto params = getEffectParams();

	auto effect = CUELFXCreateSpiralRainbowEffect(static_cast<int>(leds.size()), leds.data(), std::get<0>(params), std::get<1>(params));

	if (!effect) {
		std::cerr << "Failed to create Spiral Rainbow Effect" << std::endl;
		system("pause");
		return -1;
	}
	
	std::cout << "Playing effect...\nPress Escape to stop playback\n";
	playEffect(effect);
	system("pause");
	return 0;
}