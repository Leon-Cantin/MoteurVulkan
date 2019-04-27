#pragma once

#include "vk_globals.h"

#include <vector>

struct DescriptorWrite
{
	VkDescriptorBufferInfo bufferInfo;
	VkDescriptorImageInfo imageInfo;
	VkDescriptorType type;
	uint32_t binding;
};

struct DescriptorSet
{
	std::vector<DescriptorWrite> descriptors;

	VkDescriptorSetLayout layout;

	VkDescriptorSet set;
	VkDescriptorPool parentPool;
};

struct DescriptorWrite2
{
	VkDescriptorBufferInfo* bufferInfos;
	VkDescriptorImageInfo* imageInfos;
	uint32_t count;
	VkDescriptorType type;
	uint32_t binding;
};

struct DescriptorSet2
{
	std::vector<DescriptorWrite2> descriptors;

	VkDescriptorSetLayout layout;

	VkDescriptorSet set;
	VkDescriptorPool parentPool;
};

void createDescriptorPool(uint32_t uniformBuffersCount, uint32_t uniformBufferDynamicCount, uint32_t combinedImageSamplerCount, uint32_t storageImageCount, uint32_t maxSets, VkDescriptorPool * o_descriptorPool);
void createDescriptorSets(VkDescriptorPool descriptorPool, size_t count, DescriptorSet * descriptorSets);
void createDescriptorSets2(VkDescriptorPool descriptorPool, size_t count, DescriptorSet2 * descriptorSets);
void CreateDesciptorSetLayout(const VkDescriptorSetLayoutBinding* bindings, uint32_t count, VkDescriptorSetLayout* o_layout);
