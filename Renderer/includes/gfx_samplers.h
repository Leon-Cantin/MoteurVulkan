#pragma once

#include "vk_globals.h"

enum class eSamplers
{
	Point = 0,
	Trilinear,
	Shadow,
	Count
};
void InitSamplers();
void DestroySamplers();
VkSampler GetSampler( eSamplers samplerId );
