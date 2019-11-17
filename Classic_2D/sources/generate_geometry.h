#pragma once

#include "vk_globals.h"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <array>
#include <vector>

struct SimpleVertex {
	glm::vec3 pos;
	glm::vec2 texCoord;

	static VkVertexInputBindingDescription get_binding_description()
	{
		VkVertexInputBindingDescription bindingDescription = {};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof( SimpleVertex );
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 3> get_attribute_descriptions() {
		std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = {};
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof( SimpleVertex, pos );

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[1].offset = offsetof( SimpleVertex, texCoord );

		return attributeDescriptions;
	}
};

typedef uint32_t Index_t;
const uint32_t verticesPerQuad = 4;
const uint32_t indexesPerQuad = 6;

void GenerateQuad( std::vector<SimpleVertex> text_vertices, std::vector<Index_t> text_indices )
{
	text_vertices.resize( verticesPerQuad );
	text_indices.resize( indexesPerQuad );

	text_vertices[0] = { { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f } };
	text_vertices[1] = { { 1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f } };
	text_vertices[2] = { { 1.0f, 1.0f, 0.0f }, { 1.0f, 0.0f } };
	text_vertices[3] = { { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f } };

	text_indices[0] = 0;
	text_indices[1] = 1;
	text_indices[2] = 2;
	text_indices[3] = 1;
	text_indices[4] = 2;
	text_indices[5] = 3;
}