#pragma once

typedef void(*TickCallback)(float deltaTime, void* objectPtr);

void RegisterTickFunction(TickCallback tickCallback, void* objectPtr = nullptr);
void TickUpdate(float deltaTime);
