#include "descriptors.h"

#include <stdexcept>
#include <assert.h>
#include <array>

void createDescriptorPool(uint32_t uniformBuffersCount, uint32_t uniformBufferDynamicCount, uint32_t combinedImageSamplerCount, uint32_t storageImageCount, uint32_t sampledImageCount, uint32_t maxSets, VkDescriptorPool * o_descriptorPool)
{
	std::array<VkDescriptorPoolSize, 5> poolSizes = {};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = uniformBuffersCount;
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = combinedImageSamplerCount;
	poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	poolSizes[2].descriptorCount = storageImageCount;
	poolSizes[3].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	poolSizes[3].descriptorCount = uniformBufferDynamicCount;
	poolSizes[4].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	poolSizes[4].descriptorCount = sampledImageCount;

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = maxSets;
	poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

	if (vkCreateDescriptorPool(g_vk.device, &poolInfo, nullptr, o_descriptorPool) != VK_SUCCESS)
		throw std::runtime_error("failed to create descriptor pool!");
}

void CreateDescriptorSets( VkDescriptorPool descriptorPool, size_t count, VkDescriptorSetLayout * descriptorSetLayouts, VkDescriptorSet* o_descriptorSets)
{
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = static_cast< uint32_t >(count);
	allocInfo.pSetLayouts = descriptorSetLayouts;

	if( vkAllocateDescriptorSets( g_vk.device, &allocInfo, o_descriptorSets ) != VK_SUCCESS )
		throw std::runtime_error( "failed to allocate descriptor sets!" );
}

void UpdateDescriptorSets( size_t writeDescriptorSetsCount, const WriteDescriptorSet* writeDescriptorSets, VkDescriptorSet* descriptorSets )
{
	std::vector<VkWriteDescriptorSet> vkWriteDescriptorSets;
	for( uint32_t i = 0; i < writeDescriptorSetsCount; ++i )
	{
		VkDescriptorSet descriptorSet = descriptorSets[i];
		const WriteDescriptorSet* writeDescriptorSet = &writeDescriptorSets[i];
		for( uint32_t j = 0; j < writeDescriptorSet->count; ++j )
		{
			const WriteDescriptor* writeDescriptor = &writeDescriptorSet->writeDescriptors[j];
			VkWriteDescriptorSet writeDescriptorSet = {};
			writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSet.dstSet = descriptorSet;
			writeDescriptorSet.dstBinding = writeDescriptor->dstBinding;
			writeDescriptorSet.dstArrayElement = 0;
			writeDescriptorSet.descriptorType = writeDescriptor->type;
			writeDescriptorSet.descriptorCount = writeDescriptor->count;
			writeDescriptorSet.pBufferInfo = writeDescriptor->pBufferInfos;
			writeDescriptorSet.pImageInfo = writeDescriptor->pImageInfos;
			writeDescriptorSet.pTexelBufferView = nullptr; // Optional

			vkWriteDescriptorSets.push_back( writeDescriptorSet );
		}
	}

	vkUpdateDescriptorSets( g_vk.device, static_cast< uint32_t >(vkWriteDescriptorSets.size()), vkWriteDescriptorSets.data(), 0, nullptr );
}

void CreateDesciptorSetLayout( const VkDescriptorSetLayoutBinding* bindings, uint32_t count, VkDescriptorSetLayout* o_layout )
{
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = count;
	layoutInfo.pBindings = bindings;

	//Describe complete set of resources available (image, sampler, ubo, constants, ...)
	if (vkCreateDescriptorSetLayout(g_vk.device, &layoutInfo, nullptr, o_layout) != VK_SUCCESS)
		throw std::runtime_error("failed to create descriptor set layout!");
}