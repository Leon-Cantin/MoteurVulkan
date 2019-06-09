#pragma once

#include <string>
#include <vector>

namespace ConCom
{
	typedef void(*CommandCallback) (const std::string*, uint32_t);

	void Init();
	void OpenConsole();
	void SubmitCommand(const std::string& command);
	void RegisterCommand(const std::string& command_name, CommandCallback callback);

	bool isOpen();
	std::string GetViewableString();
}