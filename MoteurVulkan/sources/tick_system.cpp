#include "tick_system.h"

#include <array>
#include <assert.h>

#define MAX_TICK_CALLBACK 256

std::array<std::pair<TickCallback, void*>, MAX_TICK_CALLBACK> callbacks;
uint32_t callbackSize = 0;

void RegisterTickFunction(TickCallback tickCallback, void* objectPtr)
{
	assert(callbackSize != MAX_TICK_CALLBACK);
	callbacks[callbackSize++] = std::make_pair(tickCallback, objectPtr);
}

void TickUpdate(float deltaTime)
{
	for (uint32_t i = 0; i < callbackSize; ++i) {
		std::pair<TickCallback, void*>& callback = callbacks[i];
		callback.first(deltaTime, callback.second);
	}
}