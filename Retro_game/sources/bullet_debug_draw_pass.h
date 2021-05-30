#pragma once

#include "vk_globals.h"

#include "material.h"
#include "scene_frame_data.h"
#include "frame_graph.h"

R_HW::GpuPipelineLayout GetBtDebugPipelineLayout();
R_HW::GpuPipelineStateDesc GetBtDebugPipelineState();

void BtDebugRecordDrawCommandsBuffer( R_HW::GfxCommandBuffer graphicsCommandBuffer, const FG::TaskInputData& inputData );
