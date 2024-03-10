#pragma once

#include "Module.h"

class PlatformUtils : public Module<PlatformUtils>
{
	SDL_Joystick* m_joystick;
	SDL_Window* m_window;

public:
	void Startup();

	bool PollSystemEvents();
	bool ShowMessageBox(const string& string);
	float GetJoystickAxis(int idx);
};

