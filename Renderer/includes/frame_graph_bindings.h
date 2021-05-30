#pragma once

#include "frame_graph.h"

namespace FG
{
	void AddResourcesToInputBuffer( FrameGraph* frameGraph, std::array< GpuInputData, SIMULTANEOUS_FRAMES>& inputBuffers );
	void CreateTechniques( FrameGraph* frameGraph, R_HW::GfxDescriptorPool descriptorPool );
	void UpdateTechniqueDescriptorSets( const FG::FrameGraph* frameGraph, const std::array< GpuInputData, SIMULTANEOUS_FRAMES>& inputBuffers, const R_HW::GfxImage& dummyImage );
	R_HW::GfxImage CreateDummyImage();
}
