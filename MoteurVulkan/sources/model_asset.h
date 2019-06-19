#pragma once
#include "vk_globals.h"

#include <glm/vec3.hpp>
#include <glm/vec2.hpp>

#include <array>
#include <vector>

//TODO: a lot of this should be in the material
//TODO: copied from glTF_loader.cpp
enum class eVIDataElementType : uint8_t
{
	BYTE = 0,
	UNSIGNED_BYTE,
	SHORT,
	UNSIGNED_SHORT,
	UNSIGNED_INT,
	FLOAT,
	ELEMENT_TYPE_COUNT
};
const char COMPONENT_TYPE_SIZES[] = { 1, 1, 2, 2, 4, 4 };

enum class eVIDataType : uint8_t
{
	POSITION = 0,
	NORMAL,
	TANGENT,
	COLOR,
	TEX_COORD,
	VI_DATA_TYPE_COUNT
};

struct VIDesc
{
	eVIDataType dataType;
	eVIDataElementType elementType;
	unsigned char elementsCount;

	inline bool operator==( const VIDesc& other ) const
	{
		return this->dataType == other.dataType &&
		this->elementsCount == other.elementsCount &&
		this->elementType == other.elementType;
	}
};

struct VIBinding
{
	VIDesc desc;
	unsigned char location;
};

static const VIBinding positionBinding	= { eVIDataType::POSITION, eVIDataElementType::FLOAT, 3, 0 };
static const VIBinding normalBinding	= { eVIDataType::NORMAL, eVIDataElementType::FLOAT, 3, 3 };
static const VIBinding tangentBinding	= { eVIDataType::TANGENT, eVIDataElementType::FLOAT, 3, 4 };
static const VIBinding colorBinding		= { eVIDataType::COLOR, eVIDataElementType::FLOAT, 3, 1 };
static const VIBinding texCoordBinding	= { eVIDataType::TEX_COORD, eVIDataElementType::FLOAT, 2, 2 };

static const VIBinding VIBindings[] = { positionBinding, colorBinding, texCoordBinding, normalBinding, tangentBinding };
static const uint32_t viBindingCount = 5;
void get_binding_description( const VIBinding * bindingsDescs, uint32_t count, VkVertexInputBindingDescription* VIBDescs, VkVertexInputAttributeDescription* VIADescs );
uint32_t GetBindingDescription( VkVertexInputBindingDescription* VIBDescs, VkVertexInputAttributeDescription* VIADescs );

struct GfxModelVertexInput
{
	VIDesc desc;
	VkBuffer vertAttribBuffers;
	VkDeviceMemory vertAttribBuffersMemory;
};

struct GfxModel
{
	GfxModelVertexInput vertAttribBuffers[(uint8_t)eVIDataType::VI_DATA_TYPE_COUNT];
	uint32_t vertexCount;

	VkBuffer indexBuffer;
	VkDeviceMemory indicesMemory;
	uint32_t indexCount;
};

struct GfxModelCreationData
{
	VIDesc desc;
	uint8_t* data;
	uint64_t vertexCount;
};

void CreateGfxModel( const std::vector<GfxModelCreationData>& creationData, const std::vector<uint32_t>& indices, GfxModel& o_modelAsset);
void LoadGenericModel(const char * filename, GfxModel& o_modelAsset, size_t hackModelIndex);
void DestroyGfxModel(GfxModel& o_modelAsset);