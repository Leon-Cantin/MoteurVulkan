#pragma once

#include "frame_graph.h"

namespace FG
{
	void SetupInputBuffers( FrameGraph* frameGraph, std::array< GpuInputData, SIMULTANEOUS_FRAMES>& inputBuffers );
	void CreateTechniques( FrameGraph* frameGraph, GfxDescriptorPool descriptorPool );
	void UpdateTechniqueDescriptorSets( const FrameGraph* frameGraph, const std::array< GpuInputData, SIMULTANEOUS_FRAMES>& inputBuffers );
}
