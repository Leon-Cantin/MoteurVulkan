#pragma once

#include "vk_globals.h"

#include "material.h"
#include "scene_frame_data.h"
#include "frame_graph.h"

GpuPipelineLayout GetGeoPipelineLayout();
GpuPipelineStateDesc GetGeoPipelineState();

void GeometryRecordDrawCommandsBuffer( GfxCommandBuffer graphicsCommandBuffer, const FG::TaskInputData& inputData );
