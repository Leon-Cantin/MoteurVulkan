#pragma once

#include "vk_globals.h"

#include <vector>

struct WriteDescriptor
{
	uint32_t dstBinding;
	uint32_t count;
	VkDescriptorType type;
	VkDescriptorBufferInfo* pBufferInfos;
	VkDescriptorImageInfo* pImageInfos;
};

struct WriteDescriptorSet
{
	WriteDescriptor* writeDescriptors;
	uint32_t count;
};

void createDescriptorPool(uint32_t uniformBuffersCount, uint32_t uniformBufferDynamicCount, uint32_t combinedImageSamplerCount, uint32_t storageImageCount, uint32_t maxSets, VkDescriptorPool * o_descriptorPool);
void CreateDesciptorSetLayout(const VkDescriptorSetLayoutBinding* bindings, uint32_t count, VkDescriptorSetLayout* o_layout);
void CreateDescriptorSets( VkDescriptorPool descriptorPool, size_t count, VkDescriptorSetLayout * descriptorSetLayouts, VkDescriptorSet* o_descriptorSets );
void UpdateDescriptorSets( size_t writeDescriptorSetsCount, const WriteDescriptorSet* writeDescriptorSets, VkDescriptorSet* descriptorSets );
