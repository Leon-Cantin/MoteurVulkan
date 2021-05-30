#pragma once
#include "vk_globals.h"

#include <glm/vec3.hpp>
#include <glm/vec2.hpp>

#include <array>
#include <vector>

//TODO I'd like to make this a class, but it doesn't convert implicitly to VIDataType
enum class eVIDataType : R_HW::VIDataType
{
	POSITION = 0,
	NORMAL,
	TANGENT,
	COLOR,
	TEX_COORD,
	VI_DATA_TYPE_COUNT
};

static const std::vector<R_HW::VIBinding> VIBindingsFullModel = {
	{ (R_HW::VIDataType)eVIDataType::POSITION, R_HW::eVIDataElementType::FLOAT, 3, 0 },
	{ (R_HW::VIDataType)eVIDataType::COLOR, R_HW::eVIDataElementType::FLOAT, 3, 1 },
	{ (R_HW::VIDataType)eVIDataType::TEX_COORD, R_HW::eVIDataElementType::FLOAT, 2, 2 },
	{ (R_HW::VIDataType)eVIDataType::NORMAL, R_HW::eVIDataElementType::FLOAT, 3, 3 },
	{ (R_HW::VIDataType)eVIDataType::TANGENT, R_HW::eVIDataElementType::FLOAT, 3, 4 }
};

static const std::vector<R_HW::VIBinding> VIBindingsSimpleModel = {
	{ ( R_HW::VIDataType )eVIDataType::POSITION, R_HW::eVIDataElementType::FLOAT, 3, 0 },
	{ ( R_HW::VIDataType )eVIDataType::TEX_COORD, R_HW::eVIDataElementType::FLOAT, 2, 1 },
};

static const std::vector<R_HW::VIBinding> VIBindingsMeshOnly = {
	{ (R_HW::VIDataType )eVIDataType::POSITION, R_HW::eVIDataElementType::FLOAT, 3, 0 },
};

static const std::vector<R_HW::VIBinding> VIBindings_PosColUV = {
	{ (R_HW::VIDataType )eVIDataType::POSITION, R_HW::eVIDataElementType::FLOAT, 3, 0 },
	{ (R_HW::VIDataType )eVIDataType::COLOR, R_HW::eVIDataElementType::FLOAT, 3, 1 },
	{ (R_HW::VIDataType )eVIDataType::TEX_COORD, R_HW::eVIDataElementType::FLOAT, 2, 2 }
};

static const std::vector<R_HW::VIBinding> VIBindingLayout_PosCol = {
	{ (R_HW::VIDataType )eVIDataType::POSITION, R_HW::eVIDataElementType::FLOAT, 3, 0 },
	{ (R_HW::VIDataType )eVIDataType::COLOR, R_HW::eVIDataElementType::FLOAT, 3, 1 },
};

struct GfxModelVertexInput
{
	R_HW::VIDesc desc;
	R_HW::GpuBuffer buffer;
};

struct GfxModel
{
	GfxModelVertexInput vertAttribBuffers[(R_HW::VIDataType)eVIDataType::VI_DATA_TYPE_COUNT];
	uint32_t vertexCount;

	R_HW::GpuBuffer indexBuffer;
	uint32_t indexCount;
	R_HW::GfxIndexType indexType;
};

GfxModelVertexInput* GetVertexInput( GfxModel& gfxModel, eVIDataType dataType );
const GfxModelVertexInput* GetVertexInput( const GfxModel& gfxModel, eVIDataType dataType );
GfxModel CreateGfxModel( const std::vector<R_HW::VIDesc>& viDescs, size_t vertexCount, size_t indiceCount, uint8_t indexTypeSize );
GfxModel CreateGfxModel( const std::vector<R_HW::VIDesc>& viDescs, const std::vector<void*>& data, size_t vertexCount, const void* indicesData, size_t indiceCount, uint8_t indexTypeSize, R_HW::I_BufferAllocator* allocator );
void DestroyGfxModel(GfxModel& o_modelAsset);