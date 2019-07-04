#include "material.h"

#include "renderpass.h"
#include "vk_debug.h"
#include "vk_buffer.h"
#include "vk_commands.h"
#include "..\shaders\shadersCommon.h"

#include <array>

//Don't recall this if the technique is the same
void BeginTechnique( VkCommandBuffer commandBuffer, const Technique* technique, size_t currentFrame )
{
	vkCmdBindPipeline( commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, technique->pipeline );
	vkCmdBindDescriptorSets( commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, technique->pipelineLayout, RENDERPASS_SET, 1, &technique->renderPass_descriptor[currentFrame], 0, nullptr );
}

void CmdDrawTechnique( VkCommandBuffer commandBuffer, const Technique* technique, const SceneInstanceSet* instanceSet, const GfxModel* modelAsset, uint32_t currentFrame )
{
	/*vkCmdBindDescriptorSets( commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, technique->pipelineLayout, INSTANCE_SET, 1,
		&technique->instance_descriptor[currentFrame], 1, &instanceSet->geometryBufferOffsets[currentFrame] );
	CmdDrawIndexed( commandBuffer, *modelAsset );*/
}

void Destroy( GfxMaterial* material )
{
	for( Technique& technique : material->techniques )
	{
		vkDestroyPipeline( g_vk.device, technique.pipeline, nullptr );
		vkDestroyPipelineLayout( g_vk.device, technique.pipelineLayout, nullptr );
		vkDestroyDescriptorSetLayout( g_vk.device, technique.renderpass_descriptor_layout, nullptr );
		vkDestroyDescriptorSetLayout( g_vk.device, technique.instance_descriptor_layout, nullptr );
	}
}


void CreateDescriptorSetLayout( const TechniqueDescriptorSetDesc * desc, VkDescriptorSetLayout * o_setLayout )
{
	std::array<VkDescriptorSetLayoutBinding, 8> tempBindings;
	uint32_t count = 0;

	for( uint32_t i = 0; i < desc->buffersCount; ++i, ++count )
	{
		const TechniqueDataBinding* dataBinding = &desc->dataBindings[i];
		const TechniqueDataEntry* dataEntry = &techniqueDataEntries[static_cast< size_t >(dataBinding->name)];

		tempBindings[count].binding = dataBinding->binding;
		tempBindings[count].descriptorCount = dataEntry->count;
		tempBindings[count].descriptorType = dataEntry->descriptorType;
		tempBindings[count].stageFlags = dataBinding->stageFlags;
		tempBindings[count].pImmutableSamplers = nullptr;
	}

	for( uint32_t i = 0; i < desc->imagesCount; ++i, ++count )
	{
		const TechniqueDataImageBinding* dataBinding = &desc->dataImageBindings[i];
		const TechniqueDataEntryImage* dataEntry = &techniqueDataEntryImages[static_cast< size_t >(dataBinding->name)];

		tempBindings[count].binding = dataBinding->binding;
		tempBindings[count].descriptorCount = dataEntry->count;
		tempBindings[count].descriptorType = dataEntry->descriptorType;
		tempBindings[count].stageFlags = dataBinding->stageFlags;
		tempBindings[count].pImmutableSamplers = nullptr;
	}

	CreateDesciptorSetLayout( tempBindings.data(), count, o_setLayout );
}

void CreateDescriptorSet( const InputBuffers* inputData, const TechniqueDescriptorSetDesc* descriptorSetDesc, VkDescriptorSetLayout descriptorSetLayout, VkDescriptorPool descriptorPool, VkDescriptorSet* o_descriptorSet )
{
	CreateDescriptorSets( descriptorPool, 1, &descriptorSetLayout, o_descriptorSet );

	assert( descriptorSetDesc->buffersCount + descriptorSetDesc->imagesCount <= 8 );
	WriteDescriptor writeDescriptors[8];
	uint32_t writeDescriptorsCount = 0;
	WriteDescriptorSet writeDescriptorSet = { writeDescriptors, descriptorSetDesc->buffersCount + descriptorSetDesc->imagesCount };

	//Fill in buffer
	VkDescriptorBufferInfo descriptorBuffersInfos[16];
	uint32_t descriptorBuffersInfosCount = 0;
	for( uint32_t i = 0; i < descriptorSetDesc->buffersCount; ++i )
	{
		const TechniqueDataBinding* bufferBinding = &descriptorSetDesc->dataBindings[i];
		const TechniqueDataEntry* techniqueDataEntry = &techniqueDataEntries[static_cast< size_t >(bufferBinding->name)];
		VkBuffer* buffers = GetBuffer( inputData, bufferBinding->name );
		uint32_t bufferStart = descriptorBuffersInfosCount;
		for( uint32_t descriptorIndex = 0;  descriptorIndex < techniqueDataEntry->count; ++descriptorIndex )
		{
			assert( descriptorBuffersInfosCount < 16 );
			descriptorBuffersInfos[descriptorBuffersInfosCount++] = { buffers[descriptorIndex], 0, VK_WHOLE_SIZE };
		}
		writeDescriptors[writeDescriptorsCount++] = { bufferBinding->binding, techniqueDataEntry->count, techniqueDataEntry->descriptorType, &descriptorBuffersInfos[bufferStart], nullptr };
	}

	//Fill in images
	VkDescriptorImageInfo descriptorImagesInfos[16];
	uint32_t descriptorImagesInfosCount = 0;
	for( uint32_t i = 0; i < descriptorSetDesc->imagesCount; ++i )
	{
		const TechniqueDataImageBinding* imageBinding = &descriptorSetDesc->dataImageBindings[i];
		const TechniqueDataEntryImage* techniqueDataEntry = &techniqueDataEntryImages[static_cast< size_t >(imageBinding->name)];
		VkDescriptorImageInfo* images = GetImage( inputData, imageBinding->name );
		uint32_t bufferStart = descriptorImagesInfosCount;
		for( uint32_t descriptorIndex = 0; descriptorIndex < techniqueDataEntry->count; ++descriptorIndex )
		{
			assert( descriptorImagesInfosCount < 16 );
			//TODO: HACK shouldn't pass in this struct in the input buffer
			descriptorImagesInfos[descriptorImagesInfosCount++] = images[descriptorIndex];
		}
		writeDescriptors[writeDescriptorsCount++] = { imageBinding->binding, techniqueDataEntry->count, techniqueDataEntry->descriptorType, nullptr, &descriptorImagesInfos[bufferStart] };
	}


	UpdateDescriptorSets( 1, &writeDescriptorSet, o_descriptorSet );
}