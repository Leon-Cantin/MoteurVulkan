#include "profile.h"

#include <cassert>
#include <algorithm>

R_HW::GfxTimeStampQueryPool g_timeStampQueryPool;

void CreateTimeStampsQueryPool(uint32_t setCount)
{
	g_timeStampQueryPool = R_HW::GfxApiCreateTimeStampsQueryPool( setCount * Timestamp::TIMESTAMP_COUNT );
}

void DestroyTimeStampsPool()
{
	R_HW::GfxApiDestroyTimeStampsPool( g_timeStampQueryPool );
}

void CmdResetTimeStampSet( R_HW::GfxCommandBuffer commandBuffer, uint32_t set)
{
	R_HW::GfxApiCmdResetTimeStamps(commandBuffer, g_timeStampQueryPool, set * Timestamp::TIMESTAMP_COUNT, Timestamp::TIMESTAMP_COUNT);
}

void CmdWriteTimestamp( R_HW::GfxCommandBuffer commandBuffer, R_HW::GfxPipelineStageFlagBits stageBits, Timestamp timestamp, uint32_t set)
{
	R_HW::GfxApiCmdWriteTimestamp(commandBuffer, g_timeStampQueryPool, stageBits, timestamp + set * Timestamp::TIMESTAMP_COUNT);
}

float GetTimestampsDelta(Timestamp first, Timestamp last, uint32_t set)
{
	assert(last > first);
	uint64_t values[2];

	if( ( first + 1 ) == last )
		R_HW::GfxApiGetTimeStampResults( g_timeStampQueryPool, first + set * Timestamp::TIMESTAMP_COUNT, 2, values );
	else
	{
		R_HW::GfxApiGetTimeStampResults( g_timeStampQueryPool, first + set * Timestamp::TIMESTAMP_COUNT, 1, &values[0] );
		R_HW::GfxApiGetTimeStampResults( g_timeStampQueryPool, last + set * Timestamp::TIMESTAMP_COUNT, 1,&values[1] );
	}

	float miliseconds = (values[1] - values[0]) / 1000000.0f;
	return miliseconds;
}