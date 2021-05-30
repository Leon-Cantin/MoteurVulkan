#pragma once

#include "frame_graph.h"

namespace FG
{
	void AddResourcesToInputBuffer( FrameGraph* frameGraph, std::array< GpuInputData, SIMULTANEOUS_FRAMES>& inputBuffers );
	void CreateTechniques( FrameGraph* frameGraph, GfxDescriptorPool descriptorPool );
	void UpdateTechniqueDescriptorSets( const FG::FrameGraph* frameGraph, const std::array< GpuInputData, SIMULTANEOUS_FRAMES>& inputBuffers, const GfxImage& dummyImage );
	GfxImage CreateDummyImage();
}
