#pragma once

#include "vk_globals.h"
#include "gfx_model.h"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <array>
#include <vector>

typedef uint32_t Index_t;
const uint32_t verticesPerQuad = 4;
const uint32_t indexesPerQuad = 6;

GfxModel GenerateQuad( float width, float height, R_HW::I_BufferAllocator* allocator )
{
	std::vector<glm::vec3> text_vertices_pos;
	text_vertices_pos.resize( verticesPerQuad );
	std::vector<glm::vec3> text_vertices_color;
	text_vertices_color.resize( verticesPerQuad );
	std::vector<glm::vec2> text_vertices_uv;
	text_vertices_uv.resize( verticesPerQuad );

	std::vector<Index_t> text_indices;
	text_indices.resize( indexesPerQuad );

	float vertexOffset_x = width / 2.0f;
	float vertexOffset_y = height / 2.0f;
	text_vertices_pos[0] = { -vertexOffset_x, -vertexOffset_y, 0.0f };
	text_vertices_color[0] = { 1.0f, 0.0f, 0.0f };
	text_vertices_uv[0] = { 0.0f, 1.0f };

	text_vertices_pos[1] = { vertexOffset_x, -vertexOffset_y, 0.0f };
	text_vertices_color[1] = { 0.0f, 1.0f, 0.0f };
	text_vertices_uv[1] = { 1.0f, 1.0f };

	text_vertices_pos[2] = { vertexOffset_x, vertexOffset_y, 0.0f };
	text_vertices_color[2] = { 0.0f, 0.0f, 1.0f };
	text_vertices_uv[2] = { 1.0f, 0.0f };

	text_vertices_pos[3] = { -vertexOffset_x, vertexOffset_y, 0.0f };
	text_vertices_color[3] = { 1.0f, 1.0f, 1.0f };
	text_vertices_uv[3] = { 0.0f, 0.0f };

	text_indices[0] = 0;
	text_indices[1] = 1;
	text_indices[2] = 2;
	text_indices[3] = 2;
	text_indices[4] = 3;
	text_indices[5] = 0;

	std::vector<R_HW::VIDesc> modelVIDescs = {
		{ (R_HW::VIDataType )eVIDataType::POSITION, R_HW::eVIDataElementType::FLOAT, 3 },
		{ (R_HW::VIDataType )eVIDataType::COLOR, R_HW::eVIDataElementType::FLOAT, 3 },
		{ (R_HW::VIDataType )eVIDataType::TEX_COORD, R_HW::eVIDataElementType::FLOAT, 2 },
	};

	std::vector<void*> modelData = {
		text_vertices_pos.data(),
		text_vertices_color.data(),
		text_vertices_uv.data(),
	};

	return CreateGfxModel( modelVIDescs, modelData, verticesPerQuad, text_indices.data(), indexesPerQuad, sizeof( Index_t ), allocator );
}

GfxModel GenerateQuad( float size, R_HW::I_BufferAllocator* allocator )
{
	return GenerateQuad( size, size, allocator );
}