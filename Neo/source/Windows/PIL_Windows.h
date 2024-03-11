#pragma once

// Platform Interface Layer
//
// miscellaneous utility routines that need platform specific implementations

#include "Module.h"

class PIL : public Module<PIL>
{
	SDL_Joystick* m_joystick = nullptr;
	SDL_Window* m_window = nullptr;

public:
	void Startup();

	bool PollSystemEvents();
	bool ShowMessageBox(const string& string);
	float GetJoystickAxis(int idx);
};

