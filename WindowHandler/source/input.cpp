#include "input.h"

#include <unordered_map>
#include <vector>
#include <array>
#include <assert.h>
#include <cstring>
#include <cstdint>

#include "window_handler.h"
//TODO: this is starting to be a mess. especially init and cleanup
namespace IH
{
	typedef void(*ActionCallback)(void);

	struct Action {
		std::array< std::vector<ActionCallback>, eKeyState::Count > callbacks;
		eKeyState state;
	};

	std::unordered_map<std::string, Action> name_action_map;
	std::unordered_map<uint32_t, std::string> input_action_map;
	std::vector<CharacterCallback> character_callbacks;

	const uint32_t HELD_KEYS_MAX_SIZE = 256;
	int heldKeys[HELD_KEYS_MAX_SIZE];
	uint32_t heldKeysSize = 0;
	constexpr uint8_t PRESSED_STATE = 0xFF;
	uint8_t lastKeysState[HELD_KEYS_MAX_SIZE];
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
			std::string actionName = it->second;
			auto action_it = name_action_map.find( actionName );
			if( action_it != name_action_map.end() )
			{
				action_it->second.state = keyStatus;
				const auto& callbacks = action_it->second.callbacks[keyStatus];
				for( size_t i = 0; i < callbacks.size(); ++i )
					callbacks[i]();
			}
		}
	}

	void RegisterAction( const std::string& name, uint32_t input )
	{
		name_action_map.insert( { name, Action() } );
		input_action_map.insert( { input, name } );
	}

	void BindAction( const std::string& actionName, eKeyState actionState, ActionCallback input )
	{
		name_action_map[actionName].callbacks[actionState].push_back( input );
	}

	void RegisterCharacterCallback(CharacterCallback callback)
	{
		character_callbacks.push_back(callback);
	}

	void BindInputToAction(const std::string& actionName, uint32_t input)
	{
		auto it = name_action_map.find( actionName );
		input_action_map.insert( { input, actionName } );
	}

	eKeyState GetActionState( const std::string& actionName )
	{
		return name_action_map[actionName].state;
	}

	/*static void ResetReleasedKeys()
	{
		for( auto it : name_action_map )
		{
			if( it.second.state == eKeyState::Released )
				it.second.state = eKeyState::None;
		}
	}*/

	static void PollEvents()
	{
		//ResetReleasedKeys();

		constexpr uint32_t kbStateSize = 256;
		uint8_t kbState[kbStateSize];
		//TODO cross platform
		GetKeyboardState(kbState);
		constexpr uint8_t KEY_PRESSED_MASK = 1 << 7;
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

	void CleanupInputs()
	{
		heldKeysSize = 0;
		memset( heldKeys, UNKNOWN, sizeof( heldKeys ) );
		memset( lastKeysState, 0, sizeof( lastKeysState ) );
		inputsActive = true;

		name_action_map.clear();
		input_action_map.clear();
		character_callbacks.clear();
	}

	void InitInputs()
	{
		CleanupInputs();
		WH::SetCharCallback(character_callback);
	}
}