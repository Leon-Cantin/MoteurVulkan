#include "descriptors.h"

#include <stdexcept>
#include <assert.h>
#include <array>

//TODO: Replace dynamic buffers (vulkan's concept) with root descriptors (from DX12)

void CreateDescriptorPool(uint32_t uniformBuffersCount, uint32_t uniformBufferDynamicCount, uint32_t combinedImageSamplerCount, uint32_t storageImageCount, uint32_t sampledImageCount, uint32_t maxSets, VkDescriptorPool * o_descriptorPool)
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

	if (vkCreateDescriptorPool(g_vk.device.device, &poolInfo, nullptr, o_descriptorPool) != VK_SUCCESS)
		throw std::runtime_error("failed to create descriptor pool!");
}

void CreateDescriptorTables( GfxDescriptorPool descriptorPool, uint32_t count, GfxDescriptorTableLayout * descriptorSetLayouts, GfxDescriptorTable* o_descriptorSets)
{
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = count;
	allocInfo.pSetLayouts = descriptorSetLayouts;

	if( vkAllocateDescriptorSets( g_vk.device.device, &allocInfo, o_descriptorSets ) != VK_SUCCESS )
		throw std::runtime_error( "failed to allocate descriptor sets!" );
}

void UpdateDescriptorTables( size_t writeDescriptorSetsCount, const WriteDescriptorTable* writeDescriptorSets, GfxDescriptorTable* descriptorSets )
{
	std::vector<VkWriteDescriptorSet> vkWriteDescriptorSets;
	for( uint32_t i = 0; i < writeDescriptorSetsCount; ++i )
	{
		VkDescriptorSet descriptorSet = descriptorSets[i];
		const WriteDescriptorTable* writeDescriptorSet = &writeDescriptorSets[i];
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

	vkUpdateDescriptorSets( g_vk.device.device, static_cast< uint32_t >(vkWriteDescriptorSets.size()), vkWriteDescriptorSets.data(), 0, nullptr );
}

void CreateDesciptorTableLayout( const VkDescriptorSetLayoutBinding* bindings, uint32_t count, GfxDescriptorTableLayout* o_layout )
{
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = count;
	layoutInfo.pBindings = bindings;

	//Describe complete set of resources available (image, sampler, ubo, constants, ...)
	if (vkCreateDescriptorSetLayout(g_vk.device.device, &layoutInfo, nullptr, o_layout) != VK_SUCCESS)
		throw std::runtime_error("failed to create descriptor set layout!");
}

GfxDescriptorTableLayoutBinding CreateDescriptorTableLayoutBinding( uint32_t descriptorBindingSlot, GfxShaderStageFlags descriptorStageFlags, eDescriptorType descriptorType, eDescriptorAccess descriptorAccess, uint32_t descriptorCount )
{
	VkDescriptorSetLayoutBinding layoutBinding;

	layoutBinding.binding = descriptorBindingSlot;
	layoutBinding.descriptorCount = descriptorCount;
	layoutBinding.descriptorType = DescriptorTypeToVkType( descriptorType, descriptorAccess );
	layoutBinding.stageFlags = ToVkShaderStageFlags( descriptorStageFlags );
	layoutBinding.pImmutableSamplers = nullptr;

	return layoutBinding;
}

VkDescriptorType DescriptorTypeToVkType( eDescriptorType type, eDescriptorAccess access )
{
	bool write = access == eDescriptorAccess::WRITE;
	switch( type )
	{
	case eDescriptorType::BUFFER:
		return write ? VK_DESCRIPTOR_TYPE_STORAGE_BUFFER : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	case eDescriptorType::BUFFER_DYNAMIC:
		return write ? VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	case eDescriptorType::IMAGE:
		return write ? VK_DESCRIPTOR_TYPE_STORAGE_IMAGE : VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	case eDescriptorType::SAMPLER:
		assert( !write );
		return VK_DESCRIPTOR_TYPE_SAMPLER;
	case eDescriptorType::IMAGE_SAMPLER:
		assert( !write );
		return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	default:
		throw std::runtime_error( "Unknown descriptor type" );
	}
}