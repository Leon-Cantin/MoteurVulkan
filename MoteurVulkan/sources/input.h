#pragma once

#include <string>

typedef void(*ActionCallback)(void);

void RegisterAction(const std::string& name, ActionCallback callback);
void BindInputToAction(const std::string& actionName, uint32_t input);
void CallActionCallbacks(uint32_t input);
void DoCommands();
void AddHeldKey(uint32_t input);
void RemoveHeldKey(uint32_t input);
void InitInputs();
