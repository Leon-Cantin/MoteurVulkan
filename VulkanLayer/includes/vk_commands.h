#pragma once

#include "vk_globals.h"

#include <vector>

typedef VkCommandBuffer CommandBuffer;
typedef VkCommandPool CommandPool;

enum class QueueFlagBits : uint8_t
{
	TRANFER = 1 << 0,
	COMPUTE = 1 << 1,
	GRAPHICS = 1 << 2,
	PRESENT = 1 << 3,
};

/*struct Queue
{
	VkQueue queue;
	uint8_t queueTypeFlag;
	uint32_t queueFamilyIndex;
};

struct CreateDeviceData
{
	uint8_t* queueTypeFlags;
	uint32_t queuesCount;
};

CommandBuffer CreateCommandBuffer( CommandPool parentPool );
CommandPool CreateCommandPool( uint8_t queueTypeFlag );
Queue CreateQueue( uint8_t queueTypeFlag );
Device CreateDevice( CreateDeviceData createDeviceData );*/




VkCommandBuffer beginSingleTimeCommands();
void endSingleTimeCommands(VkCommandBuffer commandBuffer);
void CreateSingleUseCommandPool(uint32_t queueFamilyIndex, VkCommandPool* o_commandPool);
void CreateCommandPool(uint32_t queueFamilyIndex, VkCommandPool* o_commandPool);

void BeginCommandBufferRecording(VkCommandBuffer commandBuffer);
void EndCommandBufferRecording(VkCommandBuffer commandBuffer);