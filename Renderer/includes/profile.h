#pragma once

#include "vk_globals.h"

enum Timestamp
{
	COMMAND_BUFFER_START = 0,
	COMMAND_BUFFER_END,
	TIMESTAMP_COUNT,
};

void CreateTimeStampsQueryPool(uint32_t setCount);
void DestroyTimeStampsPool();
void CmdResetTimeStampSet(VkCommandBuffer commandBuffer, uint32_t set);
void CmdWriteTimestamp( VkCommandBuffer commandBuffer, GfxPipelineStageFlagBits stageBits, Timestamp timestamp, uint32_t set );
float GetTimestampsDelta(Timestamp first, Timestamp last, uint32_t set);