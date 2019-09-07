#pragma once

#include "vk_globals.h"

#include <vector>
#include "renderpass.h"

#include "model_asset.h"
#include "vk_shader.h"
#include "swapchain.h"
#include "scene_frame_data.h"
#include "material.h"

struct TextVertex {
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 texCoord;

	static VkVertexInputBindingDescription get_binding_description()
	{
		VkVertexInputBindingDescription bindingDescription = {};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(TextVertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 3> get_attribute_descriptions() {
		std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = {};
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(TextVertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(TextVertex, color);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(TextVertex, texCoord);

		return attributeDescriptions;
	}
};

struct TextZone {
	float x;
	float y;
	std::string text;
};


GpuPipelineLayout GetTextPipelineLayout();
GpuPipelineState GetTextPipelineState();
void CleanupTextRenderPass();
void TextRecordDrawCommandsBuffer(uint32_t currentFrame, const SceneFrameData* frameData, VkCommandBuffer graphicsCommandBuffer, VkExtent2D extent, const RenderPass * renderpass, const Technique * technique );
void CreateTextVertexBuffer( size_t maxCharCount);
void UpdateText( const TextZone * textZones, size_t textZonesCount, VkExtent2D surfaceExtent);
void LoadFontTexture();
const GfxImage* GetTextImage();