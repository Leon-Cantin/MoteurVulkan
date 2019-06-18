#pragma once
#include "vk_globals.h"

#include <glm/vec3.hpp>
#include <glm/vec2.hpp>

#include <array>
#include <vector>

//TODO: How do we deal with the different types. Number of elements and data type.
//TODO: use this VIBindingOrder to know where to bind each attribute. They should be an array at the position said by VIDataType.
//TODO: copied from mode_asset.cpp
enum VIDataElementType : unsigned char
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

enum VIDataType : unsigned char
{
	POSITION = 0,
	NORMAL,
	TANGENT,
	COLOR,
	TEX_COORD,
	VI_DATA_TYPE_COUNT
};

struct VIBinding
{
	VIDataType dataType;
	VIDataElementType elementType;
	unsigned char elementsCount;
	unsigned char location;
};

static const VIBinding positionBinding	= { POSITION, FLOAT, 3, 0 };
static const VIBinding normalBinding	= { NORMAL, FLOAT, 3, 3 };
static const VIBinding tangentBinding	= { TANGENT, FLOAT, 3, 4 };
static const VIBinding colorBinding		= { COLOR, FLOAT, 3, 1 };
static const VIBinding texCoordBinding	= { TEX_COORD, FLOAT, 2, 2 };

static const VIBinding VIBindingOrder[] = { positionBinding, colorBinding, texCoordBinding, normalBinding, tangentBinding };

//extern const std::vector<Vertex> static_cube_vertices;
extern const std::vector<uint32_t> static_cube_indices;

void CreateGfxModel(const std::vector<glm::vec3>& vertPos,
	const std::vector<glm::vec3>& vertNormal,
	const std::vector<glm::vec3>& vertTangent,
	const std::vector<glm::vec3>& vertColor,
	const std::vector<glm::vec2>& vertTexCoord,
	const std::vector<uint32_t>& indices,
	GfxModel& o_modelAsset);
void LoadGenericModel(const char * filename, GfxModel& o_modelAsset, size_t hackModelIndex);
void DestroyGfxModel(GfxModel& o_modelAsset);

struct GfxModel {
	VkBuffer vertPosBuffer;
	VkBuffer vertNormalBuffer;
	VkBuffer vertTangentBuffer;
	VkBuffer vertColorBuffer;
	VkBuffer vertTexCoordBuffer;

	VkDeviceMemory vertPosMem;
	VkDeviceMemory vertNormalMem;
	VkDeviceMemory vertTangentMem;
	VkDeviceMemory vertColorMem;
	VkDeviceMemory vertTexCoordMem;

	VkBuffer indexBuffer;
	VkDeviceMemory indicesMemory;


	uint32_t vertexCount;
	uint32_t indexCount;

	//Binding is only important when binding the buffers
	//Location is only used in the shader
	//So there should be something that links the location to the VIB description
	static std::array<VkVertexInputBindingDescription, 5> get_binding_description()
	{
		std::array<VkVertexInputBindingDescription, 5> bindingDescriptions = {};
		bindingDescriptions[0].binding = 0;
		bindingDescriptions[0].stride = sizeof( glm::vec3 );
		bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		bindingDescriptions[1].binding = 1;
		bindingDescriptions[1].stride = sizeof( glm::vec3 );
		bindingDescriptions[1].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		bindingDescriptions[2].binding = 2;
		bindingDescriptions[2].stride = sizeof( glm::vec2 );
		bindingDescriptions[2].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		bindingDescriptions[3].binding = 3;
		bindingDescriptions[3].stride = sizeof( glm::vec3 );
		bindingDescriptions[3].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		bindingDescriptions[4].binding = 4;
		bindingDescriptions[4].stride = sizeof( glm::vec3 );
		bindingDescriptions[4].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescriptions;
	}

	static std::array<VkVertexInputAttributeDescription, 5> get_attribute_descriptions()
	{
		std::array<VkVertexInputAttributeDescription, 5> attributeDescriptions = {};
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = 0;

		attributeDescriptions[1].binding = 1;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = 0;

		attributeDescriptions[2].binding = 2;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset = 0;

		attributeDescriptions[3].binding = 3;
		attributeDescriptions[3].location = 3;
		attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[3].offset = 0;

		attributeDescriptions[4].binding = 4;
		attributeDescriptions[4].location = 4;
		attributeDescriptions[4].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[4].offset = 0;

		return attributeDescriptions;
	}
};