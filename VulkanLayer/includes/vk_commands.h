#pragma once

#include "vk_globals.h"

#include <vector>

typedef GfxCommandBuffer CommandBuffer;
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




GfxCommandBuffer beginSingleTimeCommands();
void endSingleTimeCommands(GfxCommandBuffer commandBuffer);
void CreateSingleUseCommandPool(uint32_t queueFamilyIndex, GfxCommandPool* o_commandPool);
void CreateCommandPool(uint32_t queueFamilyIndex, GfxCommandPool* o_commandPool);
void Destroy( GfxCommandPool* commandPool );

void BeginCommandBufferRecording(GfxCommandBuffer commandBuffer);
void EndCommandBufferRecording(GfxCommandBuffer commandBuffer);