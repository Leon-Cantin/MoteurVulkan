#pragma once
#include "vk_globals.h"
#include "vk_vertex_input.h"
#include "vk_buffer.h"

#include <glm/vec3.hpp>
#include <glm/vec2.hpp>

#include <array>
#include <vector>

static const std::vector<VIBinding> VIBindingsFullModel = {
	{ eVIDataType::POSITION, eVIDataElementType::FLOAT, 3, 0 },
	{ eVIDataType::COLOR, eVIDataElementType::FLOAT, 3, 1 },
	{ eVIDataType::TEX_COORD, eVIDataElementType::FLOAT, 2, 2 },
	{ eVIDataType::NORMAL, eVIDataElementType::FLOAT, 3, 3 },
	{ eVIDataType::TANGENT, eVIDataElementType::FLOAT, 3, 4 }
};

static const std::vector<VIBinding> VIBindingsSimpleModel = {
	{ eVIDataType::POSITION, eVIDataElementType::FLOAT, 3, 0 },
	{ eVIDataType::TEX_COORD, eVIDataElementType::FLOAT, 2, 1 },
};

static const std::vector<VIBinding> VIBindingsMeshOnly = {
	{ eVIDataType::POSITION, eVIDataElementType::FLOAT, 3, 0 },
};

static const std::vector<VIBinding> VIBindings_PosColUV = {
	{ eVIDataType::POSITION, eVIDataElementType::FLOAT, 3, 0 },
	{ eVIDataType::COLOR, eVIDataElementType::FLOAT, 3, 1 },
	{ eVIDataType::TEX_COORD, eVIDataElementType::FLOAT, 2, 2 }
};

struct GfxModelVertexInput
{
	VIDesc desc;
	GpuBuffer vertexAttribBuffer;
};

struct GfxModel
{
	GfxModelVertexInput vertAttribBuffers[(uint8_t)eVIDataType::VI_DATA_TYPE_COUNT];
	uint32_t vertexCount;

	GpuBuffer indexBuffer;
	uint32_t indexCount;
};

struct GfxModelCreationData
{
	VIDesc desc;
	uint8_t* data;
	uint64_t vertexCount;
};

void CreateGfxModel( const std::vector<GfxModelCreationData>& creationData, const std::vector<uint32_t>& indices, GfxModel& o_modelAsset);
void CreateGfxModelNoData( const std::vector<GfxModelCreationData>& creationData, uint32_t indiceCount, GfxModel& o_modelAsset );
void LoadGenericModel(const char * filename, GfxModel& o_modelAsset, size_t hackModelIndex);
void DestroyGfxModel(GfxModel& o_modelAsset);