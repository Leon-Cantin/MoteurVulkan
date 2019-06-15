#include "input.h"

#include <unordered_map>
#include <vector>
#include <assert.h>

#include "window_handler.h"

namespace IH
{
	typedef void(*ActionCallback)(void);

	struct Action {
		std::vector<ActionCallback> callbacks[2];
	};

	std::unordered_map<std::string, Action> name_action_map;
	std::unordered_map<uint32_t, Action*> input_action_map;
	std::vector<CharacterCallback> character_callbacks;

	const uint32_t HELD_KEYS_MAX_SIZE = 256;
	int heldKeys[HELD_KEYS_MAX_SIZE];
	uint32_t heldKeysSize = 0;
	constexpr byte PRESSED_STATE = 0xFF;
	byte lastKeysState[HELD_KEYS_MAX_SIZE];
	bool inputsActive = true;

	static void character_callback( uint32_t codepoint)
	{
		for (auto callback : character_callbacks)
			callback(codepoint);
	}

	static void CallActionCallbacks(uint32_t input, eKeyState keyStatus)
	{
		assert(keyStatus == Pressed || keyStatus == Released);
		auto it = input_action_map.find(input);
		if (it != input_action_map.end())
		{
			Action* action = it->second;
			for (size_t i = 0; i < action->callbacks[keyStatus].size(); ++i)
				action->callbacks[keyStatus][i]();
		}
	}

	void RegisterAction(const std::string& name, eKeyState status, ActionCallback callback)
	{
		assert(status == Pressed || status == Released);
		Action &action = name_action_map[name];
		action.callbacks[status].push_back(callback);
	}

	void RegisterCharacterCallback(CharacterCallback callback)
	{
		character_callbacks.push_back(callback);
	}

	void BindInputToAction(const std::string& actionName, uint32_t input)
	{
		Action &action = name_action_map[actionName];
		input_action_map[input] = &action;
	}

	static void PollEvents()
	{
		constexpr uint32_t kbStateSize = 256;
		byte kbState[kbStateSize];
		GetKeyboardState(kbState);
		constexpr byte KEY_PRESSED_MASK = 1 << 7;
		for (uint32_t i = 0; i < kbStateSize; ++i)
		{
			if ((kbState[i] & KEY_PRESSED_MASK))
			{
				CallActionCallbacks(i, Pressed);
			}
			else if (!(kbState[i] & KEY_PRESSED_MASK) && lastKeysState[i])
			{
				CallActionCallbacks(i, Released);
			}
			lastKeysState[i] = kbState[i] & KEY_PRESSED_MASK;
		}
	}

	void DoCommands()
	{
		if(inputsActive)
			PollEvents();
	}

	void ActiveInputs(bool value)
	{
		inputsActive = value;
	}

	void InitInputs()
	{
		memset(heldKeys, UNKNOWN, sizeof(heldKeys));

		WH::SetCharCallback(character_callback);
	}
}