#include "input.h"

//TODO: get rid of this, this is just for g_window
#include "vk_framework.h"

#include <unordered_map>
#include <vector>

namespace IH
{

	typedef void(*ActionCallback)(void);

	struct Action {
		std::vector<ActionCallback> callbacks;
	};

	std::unordered_map<std::string, Action> name_action_map;
	std::unordered_map<uint32_t, Action*> input_action_map;
	std::vector<CharacterCallback> character_callbacks;

	const uint32_t HELD_KEYS_MAX_SIZE = 256;
	int heldKeys[HELD_KEYS_MAX_SIZE];
	uint32_t heldKeysSize = 0;

	static void character_callback(GLFWwindow* window, unsigned int codepoint)
	{
		for (auto callback : character_callbacks)
			callback(codepoint);
	}

	static void CallActionCallbacks(uint32_t input)
	{
		auto it = input_action_map.find(input);
		if (it != input_action_map.end())
		{
			Action* action = it->second;
			for (size_t i = 0; i < action->callbacks.size(); ++i)
				action->callbacks[i]();
		}
	}

	static void AddHeldKey(uint32_t input)
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

	static void RemoveHeldKey(uint32_t input)
	{
		for (uint32_t i = 0; i < heldKeysSize; ++i)
		{
			if (heldKeys[i] == input)
			{
				heldKeys[i] = GLFW_KEY_UNKNOWN;
				heldKeysSize -= (i == heldKeysSize - 1) ? 1 : 0;
				return;
			}
		}
	}

	static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		if (action == GLFW_PRESS)
		{
			IH::AddHeldKey(key);
			//TODO: add repeat function CallActionCallbacks(static_cast<uint32_t>(key));
		}
		else if (action == GLFW_RELEASE)
		{
			IH::RemoveHeldKey(key);
		}
	}

	void InitInputs()
	{
		memset(heldKeys, GLFW_KEY_UNKNOWN, sizeof(heldKeys));

		glfwSetCharCallback(g_window, character_callback);
		glfwSetKeyCallback(g_window, key_callback);
	}

	void RegisterAction(const std::string& name, ActionCallback callback)
	{
		Action &action = name_action_map[name];
		action.callbacks.push_back(callback);
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

	void DoCommands()
	{
		for (uint32_t i = 0; i < heldKeysSize; ++i)
		{
			int key = heldKeys[i];
			if (key != GLFW_KEY_UNKNOWN)
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
}