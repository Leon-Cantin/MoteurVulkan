#include "console_command.h"

#include <map>
#include <sstream>
#include <iostream>
#include <stdlib.h>

std::map<std::string, CommandCallback> command_map;

std::string console_string = "";

void SubmitCommand(const std::string& command)
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

void RegisterCommand(const std::string& command_name, CommandCallback callback)
{
	command_map.insert(std::pair(command_name, callback));
}

void AddConsoleChar(unsigned int chararacter)
{
	console_string.push_back(chararacter);
}

void ClearConsoleText()
{
	console_string.clear();
	memset(console_string.data(), 0, console_string.capacity() * sizeof(char));
}

void RemoveConsoleChar()
{
	if(!console_string.empty())
		console_string.erase(console_string.end() - 1);
}

void SubmitCommand()
{
	SubmitCommand(console_string.data());
}

std::string GetViewableString()
{
	std::string string = "->";
	string.append(console_string.data());
	string.append("|");
	return string;
}