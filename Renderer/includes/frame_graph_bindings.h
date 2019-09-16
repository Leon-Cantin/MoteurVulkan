#pragma once

#include "frame_graph.h"

namespace FG
{
	void CreateTechniques( FrameGraph* frameGraph, VkDescriptorPool descriptorPool, std::array< GpuInputData, SIMULTANEOUS_FRAMES>& inputBuffers );
}
