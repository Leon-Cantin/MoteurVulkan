#include "profile.h"

#include <cassert>
#include <algorithm>

VkQueryPool g_timeStampQueryPool;

void CreateTimeStampsQueryPool(uint32_t setCount)
{
	VkQueryPoolCreateInfo poolCreateInfo = {};
	poolCreateInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
	poolCreateInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
	poolCreateInfo.queryCount = setCount * Timestamp::TIMESTAMP_COUNT;
	poolCreateInfo.pipelineStatistics = 0; //Only used for pipeline statistic queries
	vkCreateQueryPool(g_vk.device.device, &poolCreateInfo, nullptr, &g_timeStampQueryPool);
}

void CmdResetTimeStampSet(VkCommandBuffer commandBuffer, uint32_t set)
{
	vkCmdResetQueryPool(commandBuffer, g_timeStampQueryPool, set * Timestamp::TIMESTAMP_COUNT, Timestamp::TIMESTAMP_COUNT);
}

void CmdWriteTimestamp(VkCommandBuffer commandBuffer, VkPipelineStageFlagBits stageBits, Timestamp timestamp, uint32_t set)
{
	vkCmdWriteTimestamp(commandBuffer, stageBits, g_timeStampQueryPool, timestamp + set * Timestamp::TIMESTAMP_COUNT);
}

void DestroyTimeStampsPool()
{
	vkDestroyQueryPool(g_vk.device.device, g_timeStampQueryPool, nullptr);
}

float GetTimestampsDelta(Timestamp first, Timestamp last, uint32_t set)
{
	assert(last > first);
	uint64_t values[2];

	if(last - first == 1)
		vkGetQueryPoolResults(g_vk.device.device, g_timeStampQueryPool, first + set * Timestamp::TIMESTAMP_COUNT, 2, sizeof(values), values, sizeof(uint64_t), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);
	else
	{
		vkGetQueryPoolResults(g_vk.device.device, g_timeStampQueryPool, first + set * Timestamp::TIMESTAMP_COUNT, 1, sizeof(uint64_t), &values[0], sizeof(uint64_t), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);
		vkGetQueryPoolResults(g_vk.device.device, g_timeStampQueryPool, last + set * Timestamp::TIMESTAMP_COUNT, 1, sizeof(uint64_t), &values[1], sizeof(uint64_t), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);
	}

	float miliseconds = (values[1] - values[0]) / 1000000.0f;
	return miliseconds;
}