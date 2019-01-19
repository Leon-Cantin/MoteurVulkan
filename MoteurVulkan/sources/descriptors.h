#pragma once

#include "vk_globals.h"

#include <vector>

struct BufferDescriptor
{
	VkDescriptorBufferInfo bufferInfo;
	uint32_t binding;
};

struct ImageDescriptor
{
	VkDescriptorImageInfo imageInfo;
	uint32_t binding;
};

struct DescriptorSet
{
	std::vector<BufferDescriptor> bufferDescriptors;
	std::vector<BufferDescriptor> dynamicBufferDescriptors;
	std::vector<ImageDescriptor> imageSamplerDescriptors;
	std::vector<ImageDescriptor> storageImageDescriptors;

	VkDescriptorSetLayout layout;

	VkDescriptorSet set;
	VkDescriptorPool parentPool;
};

void createDescriptorPool(uint32_t uniformBuffersCount, uint32_t uniformBufferDynamicCount, uint32_t combinedImageSamplerCount, uint32_t storageImageCount, uint32_t maxSets, VkDescriptorPool * o_descriptorPool);
void createDescriptorSets(VkDescriptorPool descriptorPool, size_t count, DescriptorSet * descriptorSets);
void CreateDesciptorSetLayout(const VkDescriptorSetLayoutBinding* bindings, uint32_t count, VkDescriptorSetLayout* o_layout);
