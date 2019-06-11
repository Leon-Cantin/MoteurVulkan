#include "console_command.h"

#include "input.h"

#include <map>
#include <sstream>
#include <iostream>
#include <stdlib.h>

namespace ConCom
{
	bool console_active = false;

	std::map<std::string, CommandCallback> command_map;

	std::string console_string = "";

	static void SubmitCommand(const std::string& command)
	{
		std::vector<std::string> strings;
		std::istringstream f(command.data());
		std::string s;
		while (std::getline(f, s, ' ')) {
			std::cout << s << std::endl;
			strings.push_back(s);
		}

		if (!strings.empty())
		{
			auto iterator = command_map.find(strings[0]);
			if (iterator != command_map.end())
			{
				CommandCallback callback = iterator->second;
				callback(strings.data(), strings.size());
			}
		}
	}

	static void ClearConsoleText()
	{
		console_string.clear();
		memset(console_string.data(), 0, console_string.capacity() * sizeof(char));
	}

	static void RemoveConsoleChar()
	{
		if (!console_string.empty())
			console_string.erase(console_string.end() - 1);
	}

	static void SubmitCommand()
	{
		SubmitCommand(console_string.data());
	}

	static void AddConsoleChar(unsigned int chararacter)
	{
		if(console_active)
			console_string.push_back(chararacter);
	}

	static void AcceptCallback()
	{
		if (console_active)
		{
			//TODO: multithreading concerns
			SubmitCommand();
			ClearConsoleText();
			console_active = false;
		}
	}

	static void BackspaceCallback()
	{
		if (console_active)
		{
			RemoveConsoleChar();
		}
	}

	void RegisterCommand(const std::string& command_name, CommandCallback callback)
	{
		command_map.insert(std::pair(command_name, callback));
	}

	void OpenConsole()
	{
		ClearConsoleText();
		console_active ^= true;
	}

	std::string GetViewableString()
	{
		std::string string = "->";
		string.append(console_string.data());
		string.append("|");
		return string;
	}

	void Init()
	{
		IH::RegisterCharacterCallback(AddConsoleChar);

		IH::RegisterAction("console_accept", &AcceptCallback);
		IH::BindInputToAction("console_accept", IH::ENTER);
		IH::RegisterAction("console_backspace", &BackspaceCallback);
		IH::BindInputToAction("console_backspace", IH::BACKSPACE);
	}

	bool isOpen()
	{
		return console_active;
	}
}