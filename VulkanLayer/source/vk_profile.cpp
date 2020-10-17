#include "vk_globals.h"

GfxTimeStampQueryPool GfxApiCreateTimeStampsQueryPool( uint32_t queriesCount )
{
	VkQueryPool queryPool;

	VkQueryPoolCreateInfo poolCreateInfo = {};
	poolCreateInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
	poolCreateInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
	poolCreateInfo.queryCount = queriesCount;
	poolCreateInfo.pipelineStatistics = 0; //Only used for pipeline statistic queries
	vkCreateQueryPool( g_vk.device.device, &poolCreateInfo, nullptr, &queryPool );

	return queryPool;
}

void GfxApiDestroyTimeStampsPool( GfxTimeStampQueryPool queryPool )
{
	vkDestroyQueryPool( g_vk.device.device, queryPool, nullptr );
}

void GfxApiCmdResetTimeStamps( VkCommandBuffer commandBuffer, GfxTimeStampQueryPool timeStampQueryPool, uint32_t firstQueryId, uint32_t count )
{
	vkCmdResetQueryPool( commandBuffer, timeStampQueryPool, firstQueryId, count );
}

void GfxApiGetTimeStampResults( GfxTimeStampQueryPool timeStampQueryPool, uint32_t firstQueryId, uint32_t count, uint64_t* values )
{
	vkGetQueryPoolResults( g_vk.device.device, timeStampQueryPool, firstQueryId, count, sizeof( uint64_t ) * count, values, sizeof( uint64_t ), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT );
}

void GfxApiCmdWriteTimestamp( VkCommandBuffer commandBuffer, GfxTimeStampQueryPool timeStampQueryPool, GfxPipelineStageFlagBits stageBits, uint32_t queryId )
{
	vkCmdWriteTimestamp( commandBuffer, ( VkPipelineStageFlagBits )stageBits, timeStampQueryPool, queryId );
}