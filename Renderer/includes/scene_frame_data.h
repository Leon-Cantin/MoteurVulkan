#pragma once
#include "scene_instance.h"

#include <vector>

struct SceneFrameData {
	std::vector<const SceneRenderableAsset *> renderableAssets;
};
