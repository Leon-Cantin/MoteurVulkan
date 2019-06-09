#pragma once

//TODO: get rid of this, used for GLFW inputs
#include "vk_globals.h"

#include <string>

namespace IH
{
	typedef void(*ActionCallback)(void);
	typedef void(*CharacterCallback)(unsigned int);

	void RegisterAction(const std::string& name, ActionCallback callback);
	void BindInputToAction(const std::string& actionName, uint32_t input);
	void RegisterCharacterCallback(CharacterCallback callback);
	void DoCommands();
	void InitInputs();
}