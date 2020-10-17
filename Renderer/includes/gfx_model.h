#pragma once
#include "vk_globals.h"
#include "vk_vertex_input.h"
#include "gfx_buffer.h"

#include <glm/vec3.hpp>
#include <glm/vec2.hpp>

#include <array>
#include <vector>

//TODO I'd like to make this a class, but it doesn't convert implicitly to VIDataType
enum class eVIDataType : VIDataType
{
	POSITION = 0,
	NORMAL,
	TANGENT,
	COLOR,
	TEX_COORD,
	VI_DATA_TYPE_COUNT
};

static const std::vector<VIBinding> VIBindingsFullModel = {
	{ (VIDataType)eVIDataType::POSITION, eVIDataElementType::FLOAT, 3, 0 },
	{ (VIDataType)eVIDataType::COLOR, eVIDataElementType::FLOAT, 3, 1 },
	{ (VIDataType)eVIDataType::TEX_COORD, eVIDataElementType::FLOAT, 2, 2 },
	{ (VIDataType)eVIDataType::NORMAL, eVIDataElementType::FLOAT, 3, 3 },
	{ (VIDataType)eVIDataType::TANGENT, eVIDataElementType::FLOAT, 3, 4 }
};

static const std::vector<VIBinding> VIBindingsSimpleModel = {
	{ ( VIDataType )eVIDataType::POSITION, eVIDataElementType::FLOAT, 3, 0 },
	{ ( VIDataType )eVIDataType::TEX_COORD, eVIDataElementType::FLOAT, 2, 1 },
};

static const std::vector<VIBinding> VIBindingsMeshOnly = {
	{ ( VIDataType )eVIDataType::POSITION, eVIDataElementType::FLOAT, 3, 0 },
};

static const std::vector<VIBinding> VIBindings_PosColUV = {
	{ ( VIDataType )eVIDataType::POSITION, eVIDataElementType::FLOAT, 3, 0 },
	{ ( VIDataType )eVIDataType::COLOR, eVIDataElementType::FLOAT, 3, 1 },
	{ ( VIDataType )eVIDataType::TEX_COORD, eVIDataElementType::FLOAT, 2, 2 }
};

struct GfxModelVertexInput
{
	VIDesc desc;
	GpuBuffer buffer;
};

struct GfxModel
{
	GfxModelVertexInput vertAttribBuffers[(VIDataType)eVIDataType::VI_DATA_TYPE_COUNT];
	uint32_t vertexCount;

	GpuBuffer indexBuffer;
	uint32_t indexCount;
	GfxIndexType indexType;
};

GfxModelVertexInput* GetVertexInput( GfxModel& gfxModel, eVIDataType dataType );
const GfxModelVertexInput* GetVertexInput( const GfxModel& gfxModel, eVIDataType dataType );
GfxModel CreateGfxModel( const std::vector<VIDesc>& viDescs, size_t vertexCount, size_t indiceCount, uint8_t indexTypeSize );
GfxModel CreateGfxModel( const std::vector<VIDesc>& viDescs, const std::vector<void*>& data, size_t vertexCount, const void* indicesData, size_t indiceCount, uint8_t indexTypeSize, I_BufferAllocator* allocator );
void DestroyGfxModel(GfxModel& o_modelAsset);