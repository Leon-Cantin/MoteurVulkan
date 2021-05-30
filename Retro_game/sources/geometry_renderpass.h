#pragma once

#include "vk_globals.h"

#include "material.h"
#include "scene_frame_data.h"
#include "frame_graph.h"

R_HW::GpuPipelineLayout GetGeoPipelineLayout();
R_HW::GpuPipelineStateDesc GetGeoPipelineState();

void GeometryRecordDrawCommandsBuffer( R_HW::GfxCommandBuffer graphicsCommandBuffer, const FG::TaskInputData& inputData );
