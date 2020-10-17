#include "profile.h"

#include <cassert>
#include <algorithm>

GfxTimeStampQueryPool g_timeStampQueryPool;

void CreateTimeStampsQueryPool(uint32_t setCount)
{
	g_timeStampQueryPool = GfxApiCreateTimeStampsQueryPool( setCount * Timestamp::TIMESTAMP_COUNT );
	VkQueryPoolCreateInfo poolCreateInfo = {};
}

void DestroyTimeStampsPool()
{
	GfxApiDestroyTimeStampsPool( g_timeStampQueryPool );
}

void CmdResetTimeStampSet(VkCommandBuffer commandBuffer, uint32_t set)
{
	GfxApiCmdResetTimeStamps(commandBuffer, g_timeStampQueryPool, set * Timestamp::TIMESTAMP_COUNT, Timestamp::TIMESTAMP_COUNT);
}

void CmdWriteTimestamp(VkCommandBuffer commandBuffer, GfxPipelineStageFlagBits stageBits, Timestamp timestamp, uint32_t set)
{
	GfxApiCmdWriteTimestamp(commandBuffer, g_timeStampQueryPool, stageBits, timestamp + set * Timestamp::TIMESTAMP_COUNT);
}

float GetTimestampsDelta(Timestamp first, Timestamp last, uint32_t set)
{
	assert(last > first);
	uint64_t values[2];

	if( ( first + 1 ) == last )
		GfxApiGetTimeStampResults( g_timeStampQueryPool, first + set * Timestamp::TIMESTAMP_COUNT, 2, values );
	else
	{
		GfxApiGetTimeStampResults( g_timeStampQueryPool, first + set * Timestamp::TIMESTAMP_COUNT, 1, &values[0] );
		GfxApiGetTimeStampResults( g_timeStampQueryPool, last + set * Timestamp::TIMESTAMP_COUNT, 1,&values[1] );
	}

	float miliseconds = (values[1] - values[0]) / 1000000.0f;
	return miliseconds;
}