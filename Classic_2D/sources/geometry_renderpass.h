#pragma once

#include "vk_globals.h"

#include "frame_graph.h"
#include "material.h"
#include "scene_instance.h"//TODO: only there because scene_frame_data needs to have a definition for GfxAsset
#include "scene_frame_data.h"

GpuPipelineLayout GetGeoPipelineLayout();
GpuPipelineStateDesc GetGeoPipelineState();

void GeometryRecordDrawCommandsBuffer( GfxCommandBuffer graphicsCommandBuffer, const FG::TaskInputData& inputData );
