#include "input.h"

#include "vk_framework.h"

#include <unordered_map>
#include <vector>

typedef void(*ActionCallback)(void);

struct Action{
	std::vector<ActionCallback> callbacks;
};

std::unordered_map<std::string, Action> name_action_map;
std::unordered_map<uint32_t, Action*> input_action_map;

const uint32_t HELD_KEYS_MAX_SIZE = 256;
int heldKeys[HELD_KEYS_MAX_SIZE];
uint32_t heldKeysSize = 0;

void InitInputs()
{
	memset(heldKeys, GLFW_KEY_UNKNOWN, sizeof(heldKeys));
}

void RegisterAction(const std::string& name, ActionCallback callback)
{	
	Action &action = name_action_map[name];
	action.callbacks.push_back(callback);
}

void BindInputToAction(const std::string& actionName, uint32_t input)
{
	Action &action = name_action_map[actionName];
	input_action_map[input] = &action;
}

void CallActionCallbacks(uint32_t input)
{
	auto it = input_action_map.find(input);
	if (it != input_action_map.end())
	{
		Action* action = it->second;
		for (size_t i = 0; i < action->callbacks.size(); ++i)
			action->callbacks[i]();
	}
}

void AddHeldKey(uint32_t input)
{
	for (uint32_t i = 0; i < HELD_KEYS_MAX_SIZE; ++i)
	{
		if (heldKeys[i] == GLFW_KEY_UNKNOWN)
		{
			heldKeys[i] = input;
			heldKeysSize += i >= heldKeysSize ? 1 : 0;
			return;
		}
	}
}

void RemoveHeldKey(uint32_t input)
{
	for (uint32_t i = 0; i < heldKeysSize; ++i)
	{
		if (heldKeys[i] == input)
		{
			heldKeys[i] = GLFW_KEY_UNKNOWN;
			heldKeysSize -= (i == heldKeysSize -1) ? 1 : 0;
			return;
		}
	}
}

void DoCommands()
{
	for (uint32_t i = 0; i < heldKeysSize; ++i)
	{
		int key = heldKeys[i];
		if ( key != GLFW_KEY_UNKNOWN)
		{
			auto it = input_action_map.find(key);
			if (it != input_action_map.end())
			{
				Action* action = it->second;
				for (size_t j = 0; j < action->callbacks.size(); ++j)
					action->callbacks[j]();
			}
		}
	}
}