#include "CUESDK.h"
#include "CUELFX/CUELFX.h"
#include "Shared/LFX.h"
#include "CorsairLFX/CorsairLFX.h"
#include "CorsairLayers/CorsairLayers.h"

#include <Windows.h>
#include <WinUser.h>

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

std::vector<CorsairLedPosition> getKeyboardLeds(int minX, int maxX, const CorsairLedPositions &positions)
{
	std::vector<CorsairLedPosition> leds;
	for (int i = 0, size = positions.numberOfLed; i < size; ++i) {
		const CorsairLedPosition pos = positions.pLedPosition[i];
		const int x = int(pos.left);
		if (x >= minX && x <= maxX) {
			leds.push_back(pos);
		}
	}

	return leds;
}

CorsairFrame* getFrameFunc(Guid effectId, int offset);
void freeFrameFunc(CorsairFrame *frame);

class Effect
{
public:

	explicit Effect(int deviceIndex, const std::vector<CorsairLedColor>& colors)
		: m_deviceIndex(deviceIndex), effectId(0), mColors(colors), stopEffect(false)
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
			frame->size = 1; // SubFrames count
			frame->subframes = new CorsairSubFrame; // frame that will be send to device

			CorsairSubFrame *subFrame = frame->subframes;

			subFrame->deviceIndex = m_deviceIndex;
			subFrame->size = static_cast<int>(mColors.size());
			subFrame->ledsColors = new CorsairLedColor[mColors.size()];

			std::copy(mColors.begin(), mColors.end(), subFrame->ledsColors);
			return frame;
		}
	}

	Guid effectId;
	bool stopEffect;

private:
	std::vector<CorsairLedColor> mColors;
	std::unique_ptr<CorsairEffect> mEffect;
	int m_deviceIndex = -1;
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
		for (size_t i = 0, size = size_t(frame->size); i < size; ++i) {
			delete[] frame->subframes[i].ledsColors;
		}
		delete[] frame->subframes;
		delete frame;
	}
}

int keyboardIndex()
{
	int deviceCount = CorsairGetDeviceCount();
	for (int i = 0; i < deviceCount; ++i) {
		CorsairDeviceInfo *info = CorsairGetDeviceInfo(i);
		if (info->type == CDT_Keyboard) {
			return i;
		}
	}

	return -1;
}

int getKeyboardWidth(const CorsairLedPositions &positions)
{
	int keyboardSize = 0;
	for (int i = 0, size = positions.numberOfLed; i < size; ++i) {
		int x = int(positions.pLedPosition[i].left);
		if (x > keyboardSize) {
			keyboardSize = x;
		}
	}

	return keyboardSize;
}

// The actual function that will change the color of the Led.
void changeKeyboardLed(char character, int deviceIndex)
{
	auto ledId = CorsairGetLedIdForKeyName(character);
	auto solidColor = CUELFXCreateSolidColorEffect({ 50, 150, 200 });
	std::vector<CorsairLedPosition> leds;
	auto mapping = CorsairGetLedPositionsByDeviceIndex(deviceIndex); // Returns a dictionary (structure?) where LedIds can lookup Positions.
	leds.push_back(mapping->pLedPosition[ledId]);
	CUELFXAssignEffectToLeds(solidColor->effectId, deviceIndex, 1, leds.data());
	auto solidColorId = CorsairLayersPlayEffect(solidColor, 1);

}

HHOOK _hook_keyboard;
KBDLLHOOKSTRUCT kbdStruct;

// The callback function.
LRESULT __stdcall HookCallbackKeyboard(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode >= 0)
	{
		kbdStruct = *((KBDLLHOOKSTRUCT*)lParam);
		if (wParam == WM_KEYDOWN) {
			char c = MapVirtualKey(kbdStruct.vkCode, 2);
			changeKeyboardLed(c, 0);


		}
	}
	return CallNextHookEx(_hook_keyboard, nCode, wParam, lParam);
}
//* STUFF I DON'T UNDERSTAND
void SetHook()
{
	if (!(_hook_keyboard = SetWindowsHookEx(WH_KEYBOARD_LL, HookCallbackKeyboard, NULL, 0)))
	{
		MessageBox(NULL, "Failed to install hook on keyboard!", "Error", MB_ICONERROR);
	}
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	SetHook();
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return msg.wParam;
}

//* STUFF I DON'T UNDERSTAND

int main(int argc, char *argv[])
{
	// Standard Keyboard setup stuff.
	CorsairPerformProtocolHandshake();
	if (const auto error = CorsairGetLastError()) {
		std::cerr << "Protocol Handshake failed: " << errorString(error) << std::endl;
		system("pause");
		return -1;
	}
	
	const int deviceIndex = keyboardIndex();
	if (deviceIndex < 0) {
		std::cerr << "Unable to find keyboard index" << std::endl;
		system("pause");
		return -1;
	}

	const auto ledPositions = CorsairGetLedPositionsByDeviceIndex(deviceIndex);
	if (!ledPositions) {
		std::cerr << "No led positions available" << std::endl;
		system("pause");
		return -1;
	}

	CorsairLayersInitialize(&CorsairSetLedsColorsBufferByDeviceIndex, &CorsairSetLedsColorsFlushBufferAsync);

	const auto keyboardWidth = getKeyboardWidth(*ledPositions);
	CorsairLayersInitialize(&CorsairSetLedsColorsBufferByDeviceIndex, &CorsairSetLedsColorsFlushBufferAsync);

	// First Layer of LEDs
	const std::vector<CorsairLedPosition> firstGroupOfLeds = getKeyboardLeds(0, keyboardWidth * 0.33, *ledPositions);
	auto solidColor = CUELFXCreateSolidColorEffect({ 50, 150, 200 });
	CUELFXAssignEffectToLeds(solidColor->effectId, deviceIndex, firstGroupOfLeds.size(), firstGroupOfLeds.data());

	std::cout << "Play SolidColor effect on layer 5\nPress any key to play next step...\n";
	auto solidColorId = CorsairLayersPlayEffect(solidColor, 1);


	// Second Layer of LEDs

	const std::vector<CorsairLedPosition> secondGroupOfLeds = getKeyboardLeds(keyboardWidth * 0.33, keyboardWidth * 0.66, *ledPositions);

	auto solidColor2 = CUELFXCreateSolidColorEffect({ 255, 255, 225 });
	CUELFXAssignEffectToLeds(solidColor2->effectId, deviceIndex, secondGroupOfLeds.size(), secondGroupOfLeds.data());

	auto solidColorId2 = CorsairLayersPlayEffect(solidColor2, 2);
	_getch();

	std::cout << "Playing effect...\nPress Escape to stop playback\n";

	system("pause");
	return 0;
}