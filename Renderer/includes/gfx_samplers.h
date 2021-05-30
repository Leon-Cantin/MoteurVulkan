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
R_HW::GfxApiSampler GetSampler( eSamplers samplerId );
R_HW::GfxApiSampler* GetSamplers();
