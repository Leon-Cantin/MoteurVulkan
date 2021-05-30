#include "vk_globals.h"

namespace R_HW
{
	GfxTimeStampQueryPool GfxApiCreateTimeStampsQueryPool( uint32_t queriesCount )
	{
		VkQueryPool queryPool;

		VkQueryPoolCreateInfo poolCreateInfo = {};
		poolCreateInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
		poolCreateInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
		poolCreateInfo.queryCount = queriesCount;
		poolCreateInfo.pipelineStatistics = 0; //Only used for pipeline statistic queries
		vkCreateQueryPool( g_gfx.device.device, &poolCreateInfo, nullptr, &queryPool );

		return queryPool;
	}

	void GfxApiDestroyTimeStampsPool( GfxTimeStampQueryPool queryPool )
	{
		vkDestroyQueryPool( g_gfx.device.device, queryPool, nullptr );
	}

	void GfxApiCmdResetTimeStamps( GfxCommandBuffer commandBuffer, GfxTimeStampQueryPool timeStampQueryPool, uint32_t firstQueryId, uint32_t count )
	{
		vkCmdResetQueryPool( commandBuffer, timeStampQueryPool, firstQueryId, count );
	}

	void GfxApiGetTimeStampResults( GfxTimeStampQueryPool timeStampQueryPool, uint32_t firstQueryId, uint32_t count, uint64_t* values )
	{
		vkGetQueryPoolResults( g_gfx.device.device, timeStampQueryPool, firstQueryId, count, sizeof( uint64_t ) * count, values, sizeof( uint64_t ), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT );
	}

	void GfxApiCmdWriteTimestamp( GfxCommandBuffer commandBuffer, GfxTimeStampQueryPool timeStampQueryPool, GfxPipelineStageFlagBits stageBits, uint32_t queryId )
	{
		vkCmdWriteTimestamp( commandBuffer, ( VkPipelineStageFlagBits )stageBits, timeStampQueryPool, queryId );
	}
}