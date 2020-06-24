#pragma once

#include "inputs_def.h"

#include <string>

namespace IH
{
	typedef void(*ActionCallback)(void);
	typedef void(*CharacterCallback)(unsigned int);

	void RegisterAction( const std::string& actionName, uint32_t input );
	void BindAction( const std::string& actionName, eKeyState actionState, ActionCallback input );
	void RegisterCharacterCallback(CharacterCallback callback);
	void DoCommands();
	void ActiveInputs(bool value);
	void InitInputs();
	void CleanupInputs();

	eKeyState GetActionState( const std::string& actionName );
}