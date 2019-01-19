#include "descriptors.h"

#include <stdexcept>
#include <assert.h>
#include <array>

void createDescriptorPool(uint32_t uniformBuffersCount, uint32_t uniformBufferDynamicCount, uint32_t combinedImageSamplerCount, uint32_t storageImageCount, uint32_t maxSets, VkDescriptorPool * o_descriptorPool)
{
	std::array<VkDescriptorPoolSize, 4> poolSizes = {};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = uniformBuffersCount;
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = combinedImageSamplerCount;
	poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	poolSizes[2].descriptorCount = storageImageCount;
	poolSizes[3].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	poolSizes[3].descriptorCount = uniformBufferDynamicCount;

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = maxSets;
	poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

	if (vkCreateDescriptorPool(g_vk.device, &poolInfo, nullptr, o_descriptorPool) != VK_SUCCESS)
		throw std::runtime_error("failed to create descriptor pool!");
}

void createDescriptorSets(VkDescriptorPool descriptorPool, size_t count, DescriptorSet * descriptorSets)
{
	//Extract all layouts into a single array and create descriptors
	std::vector<VkDescriptorSetLayout> layouts;
	layouts.resize(count);
	for (size_t i = 0; i < count; ++i)
		layouts[i] = descriptorSets[i].layout;

	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(count);
	allocInfo.pSetLayouts = layouts.data();

	std::vector<VkDescriptorSet> vkDescriptorSets;
	vkDescriptorSets.resize(count);
	if (vkAllocateDescriptorSets(g_vk.device, &allocInfo, vkDescriptorSets.data()) != VK_SUCCESS)
		throw std::runtime_error("failed to allocate descriptor sets!");

	//TODO: seperate the creation of sets and the update, could copy what's already on the GPU
	//TODO: simplify this to only generate one type of write, all based on the incomming structure if possible
	//Put descriptors into output struct and update it
	std::vector<VkWriteDescriptorSet> descriptorWrites;
	for (size_t i = 0; i < count; ++i)
	{
		DescriptorSet& descriptorSet = descriptorSets[i];
		descriptorSet.set = vkDescriptorSets[i];
		descriptorSet.parentPool = descriptorPool;

		//Buffers
		for (size_t j = 0; j < descriptorSet.bufferDescriptors.size(); ++j) {
			BufferDescriptor& bufferDescriptor = descriptorSet.bufferDescriptors[j];

			VkWriteDescriptorSet writeDescriptorSet = {};
			writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSet.dstSet = descriptorSet.set;
			writeDescriptorSet.dstBinding = bufferDescriptor.binding;
			writeDescriptorSet.dstArrayElement = 0;
			writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			writeDescriptorSet.descriptorCount = 1;
			writeDescriptorSet.pBufferInfo = &bufferDescriptor.bufferInfo;
			writeDescriptorSet.pImageInfo = nullptr; // Optional
			writeDescriptorSet.pTexelBufferView = nullptr; // Optional
			descriptorWrites.push_back(writeDescriptorSet);
		}

		//Buffers dynamic
		for (size_t j = 0; j < descriptorSet.dynamicBufferDescriptors.size(); ++j) {
			BufferDescriptor& dynamicDufferDescriptor = descriptorSet.dynamicBufferDescriptors[j];

			VkWriteDescriptorSet writeDescriptorSet = {};
			writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSet.dstSet = descriptorSet.set;
			writeDescriptorSet.dstBinding = dynamicDufferDescriptor.binding;
			writeDescriptorSet.dstArrayElement = 0;
			writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
			writeDescriptorSet.descriptorCount = 1;
			writeDescriptorSet.pBufferInfo = &dynamicDufferDescriptor.bufferInfo;
			writeDescriptorSet.pImageInfo = nullptr; // Optional
			writeDescriptorSet.pTexelBufferView = nullptr; // Optional
			descriptorWrites.push_back(writeDescriptorSet);
		}

		//Combined image sampler
		for (size_t j = 0; j < descriptorSet.imageSamplerDescriptors.size(); ++j) {
			ImageDescriptor& cisDescriptor = descriptorSet.imageSamplerDescriptors[j];

			VkWriteDescriptorSet writeDescriptorSet = {};
			writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSet.dstSet = descriptorSet.set;
			writeDescriptorSet.dstBinding = cisDescriptor.binding;
			writeDescriptorSet.dstArrayElement = 0;
			writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writeDescriptorSet.descriptorCount = 1;
			writeDescriptorSet.pImageInfo = &cisDescriptor.imageInfo;
			writeDescriptorSet.pBufferInfo = nullptr; // Optional
			descriptorWrites.push_back(writeDescriptorSet);
		}

		//storage image
		for (size_t j = 0; j < descriptorSet.storageImageDescriptors.size(); ++j) {
			ImageDescriptor& siDescriptor = descriptorSet.storageImageDescriptors[j];

			VkWriteDescriptorSet writeDescriptorSet = {};
			writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSet.dstSet = descriptorSet.set;
			writeDescriptorSet.dstBinding = siDescriptor.binding;
			writeDescriptorSet.dstArrayElement = 0;
			writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			writeDescriptorSet.descriptorCount = 1;
			writeDescriptorSet.pImageInfo = &siDescriptor.imageInfo;
			writeDescriptorSet.pBufferInfo = nullptr;
			descriptorWrites.push_back(writeDescriptorSet);
		}
	}

	vkUpdateDescriptorSets(g_vk.device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void CreateDesciptorSetLayout(const VkDescriptorSetLayoutBinding* bindings, uint32_t count, VkDescriptorSetLayout* o_layout)
{
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = count;
	layoutInfo.pBindings = bindings;

	//Describe complete set of resources available (image, sampler, ubo, constants, ...)
	if (vkCreateDescriptorSetLayout(g_vk.device, &layoutInfo, nullptr, o_layout) != VK_SUCCESS)
		throw std::runtime_error("failed to create descriptor set layout!");
}