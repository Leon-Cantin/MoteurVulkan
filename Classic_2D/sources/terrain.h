#pragma once

#include "gfx_model.h"
#include "gfx_asset.h"
#include "classic_2d_gfx_asset.h"
#include "scene_instance.h"
#include "asset_library.h"

struct BackgroundInstance
{
	SceneInstance instance;
	GfxAsset asset;

	int screen_width;
	int screen_height;
};

void UpdateBackgroundScrolling( BackgroundInstance* background_instance, float frame_delta_time)
{
	const float movement_speed = 200.0f;
	background_instance->instance.location.y -= movement_speed * (frame_delta_time / 1000.0f);
	if( background_instance->instance.location.y < (-background_instance->screen_height / 2.0f - 20.0f) )
		background_instance->instance.location.y = -background_instance->screen_height / 2.0f;
}

static void GenerateQuadMegaTextureUVs( const uint32_t x_index, const uint32_t y_index, const uint32_t num_sprites_x, const uint32_t num_sprites_y,
	glm::vec2* ll, glm::vec2* lr, glm::vec2* ur, glm::vec2* ul )
{
	float left = 1.0f / ( float )num_sprites_x * ( float )x_index;
	float right = 1.0f / ( float )num_sprites_x * ( float )(x_index + 1);
	float upper = 1.0f / ( float )num_sprites_y * ( float )y_index;
	float lower = 1.0f / ( float )num_sprites_y * ( float )(y_index + 1);
	*ll = { left, lower };
	*lr = { right, lower };
	*ur = { right, upper };
	*ul = { left, upper };
}

typedef uint32_t Index_t;
BackgroundInstance CreateBackgroundGfxModel( const int screen_width, const int screen_height, const uint32_t background_sprite_sheet_index, I_BufferAllocator* allocator )
{
	const unsigned int sprite_size = 20;
	const unsigned int num_sprites_x = 2;
	const unsigned int num_sprites_y = 2;
	const unsigned int river_bank_index = 0;
	const unsigned int river_index = 1;
	const unsigned int forest_index = 2;
	const float depth = 8.0f;
	const float offset = 1.0f;//Let's all have them 1 unit and scale at render time
	const int tiles_count_x = screen_width / sprite_size + 1;
	const int tiles_count_y = screen_height / sprite_size + 2;

	const unsigned int vertices_per_quad = 4;
	const unsigned int total_vertices = vertices_per_quad * tiles_count_x * tiles_count_y;
	std::vector<glm::vec3> vertices_pos;
	vertices_pos.resize( total_vertices );
	std::vector<glm::vec3> vertices_color;
	vertices_color.resize( total_vertices );
	std::vector<glm::vec2> vertices_uv;
	vertices_uv.resize( total_vertices );

	const unsigned int index_per_quad = 6;
	const unsigned int total_indices = index_per_quad * tiles_count_x * tiles_count_y;
	std::vector<Index_t> indices;
	indices.resize( total_indices );

	for( unsigned int y = 0; y < tiles_count_y; ++y )
	{
		for( unsigned int x = 0; x < tiles_count_x; ++x )
		{
			uint32_t mega_texture_index = forest_index;
			if( x > 2 )
				mega_texture_index = river_bank_index;
			if( x > 3 )
				mega_texture_index = river_index;

			const unsigned int mega_texture_y_index = mega_texture_index / num_sprites_x;
			const unsigned int mega_texture_x_index = mega_texture_index % num_sprites_x;

			const unsigned int array_offset = (x + tiles_count_x * y);
			const unsigned int vertices_array_offset = array_offset * vertices_per_quad;//TODO: probably wrapping around on y reset
			vertices_pos[vertices_array_offset + 0] = { x*offset, y*offset, 0.0f };
			vertices_pos[vertices_array_offset + 1] = { x*offset + offset, y*offset, 0.0f };
			vertices_pos[vertices_array_offset + 2] = { x*offset + offset, y*offset + offset, 0.0f };
			vertices_pos[vertices_array_offset + 3] = { x*offset, y*offset + offset, 0.0f };

			vertices_color[vertices_array_offset + 0] = { 1.0f, 0.0f, 0.0f };
			vertices_color[vertices_array_offset + 1] = { 0.0f, 1.0f, 0.0f };
			vertices_color[vertices_array_offset + 2] = { 0.0f, 0.0f, 1.0f };
			vertices_color[vertices_array_offset + 3] = { 1.0f, 1.0f, 1.0f };

			GenerateQuadMegaTextureUVs( mega_texture_x_index, mega_texture_y_index, num_sprites_x, num_sprites_y,
				&vertices_uv[vertices_array_offset + 0],
				&vertices_uv[vertices_array_offset + 1],
				&vertices_uv[vertices_array_offset + 2],
				&vertices_uv[vertices_array_offset + 3]
			);

			const unsigned int index_array_offset = array_offset * index_per_quad;
			indices[index_array_offset + 0] = vertices_array_offset + 0;
			indices[index_array_offset + 1] = vertices_array_offset + 1;
			indices[index_array_offset + 2] = vertices_array_offset + 2;
			indices[index_array_offset + 3] = vertices_array_offset + 2;
			indices[index_array_offset + 4] = vertices_array_offset + 3;
			indices[index_array_offset + 5] = vertices_array_offset + 0;
		}
	}

	std::vector<VIDesc> modelVIDescs = {
		{ ( VIDataType )eVIDataType::POSITION, eVIDataElementType::FLOAT, 3 },
		{ ( VIDataType )eVIDataType::COLOR, eVIDataElementType::FLOAT, 3 },
		{ ( VIDataType )eVIDataType::TEX_COORD, eVIDataElementType::FLOAT, 2 },
	};

	std::vector<void*> modelData = {
		vertices_pos.data(),
		vertices_color.data(),
		vertices_uv.data(),
	};

	 BackgroundInstance background_instance;
	 GfxModel* background_model = AL::RegisterGfxModel( "background", CreateGfxModel( modelVIDescs, modelData, total_vertices, indices.data(), total_indices, sizeof( Index_t ), allocator ) );
	 background_instance.asset = CreateGfxAsset( background_model, background_sprite_sheet_index );
	 background_instance.instance = { glm::vec3( -screen_width / 2.0f, -screen_height / 2.0f, 8.0f ), defaultRotation, 20.0f };
	 background_instance.screen_width = screen_width;
	 background_instance.screen_height = screen_height;

	 return background_instance;
}