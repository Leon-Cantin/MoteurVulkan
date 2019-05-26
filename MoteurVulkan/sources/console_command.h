#pragma once

#include <string>
#include <vector>

namespace ConCom
{
	typedef void(*CommandCallback) (const std::string*, uint32_t);

	void SubmitCommand(const std::string& command);
	void RegisterCommand(const std::string& command_name, CommandCallback callback);

	void AddConsoleChar(unsigned int chararacter);
	void ClearConsoleText();
	void RemoveConsoleChar();
	void SubmitCommand();
	std::string GetViewableString();
}