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

std::vector<CorsairLedId> getSubsetLeds()
{
	std::vector<CorsairLedId> leds;
	for (int i = CLK_Escape; i <= CLK_RightAlt; ++i) {
		leds.push_back(static_cast<CorsairLedId>(i));
	}

	return leds;
}

CorsairFrame* getFrameFunc(Guid effectId, int offset);
void freeFrameFunc(CorsairFrame *frame);

class Effect
{
public:

	explicit Effect(const std::vector<CorsairLedColor>& colors)
		: effectId(0), mColors(colors), stopEffect(false)
	{
	}

	virtual ~Effect() {}

	CorsairEffect* effect()
	{
		if (!mEffect) {
			mEffect = std::unique_ptr<CorsairEffect>(new CorsairEffect);
			mEffect->effectId = reinterpret_cast<Guid>(this);
			effectId = mEffect->effectId;
			mEffect->getFrameFunction = getFrameFunc;
			mEffect->freeFrameFunction = freeFrameFunc;
		}
		return mEffect.get();
	}

	virtual CorsairFrame* getFrame(Guid effectId, int offset)
	{
		if (reinterpret_cast<Effect*>(effectId) != this || stopEffect) {
			return nullptr;
		}
		else {
			CorsairFrame *frame = new CorsairFrame;
			frame->size = static_cast<int>(mColors.size());
			frame->ledsColors = new CorsairLedColor[mColors.size()];
			std::copy(mColors.begin(), mColors.end(), frame->ledsColors);
			return frame;
		}
	}

	Guid effectId;
	bool stopEffect;

private:
	std::vector<CorsairLedColor> mColors;
	std::unique_ptr<CorsairEffect> mEffect;
};

CorsairFrame* getFrameFunc(Guid effectId, int offset)
{
	if (effectId) {
		return reinterpret_cast<Effect*>(effectId)->getFrame(effectId, offset);
	}
	else {
		return nullptr;
	}
}

void freeFrameFunc(CorsairFrame *frame)
{
	if (frame) {
		delete[] frame->ledsColors;
		delete frame;
	}
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
	auto leds_subset = getSubsetLeds();


	std::cout << "Play first effect with black-color center\nPress any key to play next step...\n";
	auto base_effect = CUELFXCreateSolidColorEffect(leds.size(), leds.data(), { 255, 255, 0 });
	auto effect1Id = CorsairLayersPlayEffect(base_effect, 1);

	auto base_effect_2 = CUELFXCreateSolidColorEffect(leds_subset.size(), leds_subset.data(), { 255, 255, 225 });
	auto effect2Id = CorsairLayersPlayEffect(base_effect_2, 2);
	_getch();

	std::cout << "Playing effect...\nPress Escape to stop playback\n";
	system("pause");
	return 0;
}