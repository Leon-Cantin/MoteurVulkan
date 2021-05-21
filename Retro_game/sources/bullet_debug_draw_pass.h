#pragma once

#include "vk_globals.h"

#include "material.h"
#include "scene_frame_data.h"
#include "frame_graph.h"

GpuPipelineLayout GetBtDebugPipelineLayout();
GpuPipelineStateDesc GetBtDebugPipelineState();

void BtDebugRecordDrawCommandsBuffer( GfxCommandBuffer graphicsCommandBuffer, const FG::TaskInputData& inputData );
