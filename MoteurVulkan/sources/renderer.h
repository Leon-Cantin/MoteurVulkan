#pragma once

#include "vk_globals.h"
#include "scene_instance.h"
#include "scene_frame_data.h"
#include "swapchain.h"
#include "frame_graph.h"

#include "vk_image.h"
#include <vector>

#include <glm/mat4x4.hpp>

void InitRenderer();
void CompileFrameGraph( FG::FrameGraph( *FGScriptInitialize )(const Swapchain* swapchain) );
void CleanupRenderer();
void WaitForFrame(uint32_t currentFrame);
void draw_frame(uint32_t currentFrame, const SceneFrameData* frameData);
