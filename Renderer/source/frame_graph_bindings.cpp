#include "frame_graph_bindings.h"
#include "frame_graph_common_internal.h"

#include "descriptors.h"
#include "material.h"

namespace FG
{
	static const FG::TechniqueDataEntry* GetDataEntry( const FG::FrameGraph* frameGraph, uint32_t entryId )
	{
		const FG::TechniqueDataEntry* dataEntry = &frameGraph->imp->creationData.resources[entryId];
		assert( dataEntry->id == entryId );
		return dataEntry;
	}

	static VkDescriptorType DescriptorTypeToVkType( eDescriptorType type, eDescriptorAccess access )
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
		}
	}

	static VkDescriptorSetLayoutBinding CreateSetLayoutBinding( const TechniqueDataBinding* dataBinding, const FG::TechniqueDataEntry* dataEntry )
	{
		VkDescriptorSetLayoutBinding layoutBinding;
		layoutBinding.binding = dataBinding->binding;
		layoutBinding.descriptorCount = dataEntry->count;
		layoutBinding.descriptorType = DescriptorTypeToVkType( dataEntry->descriptorType, dataBinding->descriptorAccess );
		layoutBinding.stageFlags = dataBinding->stageFlags;
		layoutBinding.pImmutableSamplers = nullptr;
		return layoutBinding;
	}

	static void CreateDescriptorSetLayout( const FG::FrameGraph* frameGraph, const TechniqueDescriptorSetDesc * desc, VkDescriptorSetLayout * o_setLayout )
	{
		std::array<VkDescriptorSetLayoutBinding, 8> tempBindings;
		uint32_t count = 0;

		for( uint32_t i = 0; i < desc->dataBindings.size(); ++i, ++count )
		{
			const TechniqueDataBinding* dataBinding = &desc->dataBindings[i];
			const FG::TechniqueDataEntry* dataEntry = GetDataEntry( frameGraph, dataBinding->id );

			tempBindings[count] = CreateSetLayoutBinding( dataBinding, dataEntry );
		}

		CreateDesciptorSetLayout( tempBindings.data(), count, o_setLayout );
	}

	static void CreateDescriptorSet( VkDescriptorSetLayout descriptorSetLayout, VkDescriptorPool descriptorPool, VkDescriptorSet* o_descriptorSet )
	{
		CreateDescriptorSets( descriptorPool, 1, &descriptorSetLayout, o_descriptorSet );		
	}

	static void CreatePerFrameBuffer( const FG::FrameGraph* frameGraph, const FG::TechniqueDataEntry* techniqueDataEntry, const TechniqueDataBinding* dataBinding, PerFrameBuffer* o_buffer )
	{
		VkDeviceSize size;
		switch( techniqueDataEntry->descriptorType )
		{
		case eDescriptorType::BUFFER:
			size = techniqueDataEntry->resourceDesc.extent.width;
			break;
		case eDescriptorType::BUFFER_DYNAMIC:
			size = techniqueDataEntry->resourceDesc.extent.width * techniqueDataEntry->resourceDesc.extent.height;
		}

		//TODO: could have to change VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT if we write (store). Will have to check all bindings to know.
		CreatePerFrameBuffer( size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, o_buffer );
	}

	static void SetOrCreateDataIfNeeded( FG::FrameGraph* frameGraph, std::array< GpuInputData, SIMULTANEOUS_FRAMES>* inputBuffers, const TechniqueDescriptorSetDesc* descriptorSetDesc )
	{
		for( uint32_t i = 0; i < descriptorSetDesc->dataBindings.size(); ++i )
		{
			const TechniqueDataBinding* dataBinding = &descriptorSetDesc->dataBindings[i];
			const FG::TechniqueDataEntry* dataEntry = GetDataEntry( frameGraph, dataBinding->id );

			if( dataEntry->flags & eTechniqueDataEntryFlags::EXTERNAL )
				continue;

			if( IsBufferType( dataEntry->descriptorType ) )
			{
				PerFrameBuffer* buffer = &frameGraph->imp->allbuffers[dataEntry->id];
				if( buffer->memory == VK_NULL_HANDLE )
				{
					CreatePerFrameBuffer( frameGraph, dataEntry, dataBinding, buffer );
					for( size_t i = 0; i < SIMULTANEOUS_FRAMES; ++i )
						SetBuffers( &(*inputBuffers)[i], dataEntry->id, &buffer->buffers[i], 1 );
				}
			}
			else
			{
				const GfxImage* image = frameGraph->imp->GetImage( dataEntry->id ); //TODO: GetResource( (uint32_t) id ) --------------------
				VkDescriptorImageInfo* imageInfo = &frameGraph->imp->allImages[dataEntry->id];
				if( imageInfo->imageView == VK_NULL_HANDLE )
				{
					//TODO: layout might be different for different shaders, probably just need write, works so far.
					*imageInfo = { GetSampler( dataEntry->sampler ), image->imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
					for( size_t i = 0; i < SIMULTANEOUS_FRAMES; ++i )
						SetImages( &(*inputBuffers)[i], dataEntry->id, imageInfo, 1 );
				}
			}
		}
	}

	static void CreateTechnique( FG::FrameGraph* frameGraph, VkDescriptorPool descriptorPool, const RenderPass* renderpass, const FG::RenderPassCreationData* passCreationData, Technique* o_technique, std::array< GpuInputData, SIMULTANEOUS_FRAMES>& inputBuffers )
	{
		const TechniqueDescriptorSetDesc* passSet = passCreationData->frame_graph_node.passSet;
		const TechniqueDescriptorSetDesc* instanceSet = passCreationData->frame_graph_node.instanceSet;

		//Create buffers if required
		if( passSet )
			SetOrCreateDataIfNeeded( frameGraph, &inputBuffers, passSet );
		if( instanceSet )
			SetOrCreateDataIfNeeded( frameGraph, &inputBuffers, instanceSet );

		//Create descriptors
		if( passSet )
		{
			CreateDescriptorSetLayout( frameGraph, passSet, &o_technique->renderpass_descriptor_layout );
			//TODO: not all of them need one for each simultaneous frames
			for( size_t i = 0; i < SIMULTANEOUS_FRAMES; ++i )
				CreateDescriptorSet( o_technique->renderpass_descriptor_layout, descriptorPool, &o_technique->renderPass_descriptor[i] );
		}
		if( instanceSet )
		{
			CreateDescriptorSetLayout( frameGraph, instanceSet, &o_technique->instance_descriptor_layout );
			//TODO: not all of them need one for each simultaneous frames
			for( size_t i = 0; i < SIMULTANEOUS_FRAMES; ++i )
				CreateDescriptorSet( o_technique->instance_descriptor_layout, descriptorPool, &o_technique->instance_descriptor[i] );
		}
		o_technique->parentDescriptorPool = descriptorPool;

		//Create pipeline layout
		uint32_t dscriptorSetLayoutsCount = 0;
		std::array< VkDescriptorSetLayout, 2 > descriptorSetLayouts;
		if( passSet )
			descriptorSetLayouts[dscriptorSetLayoutsCount++] = o_technique->renderpass_descriptor_layout;
		if( instanceSet )
			descriptorSetLayouts[dscriptorSetLayoutsCount++] = o_technique->instance_descriptor_layout;

		VkPipelineLayoutCreateInfo pipeline_layout_info = {};
		pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_info.setLayoutCount = dscriptorSetLayoutsCount;
		pipeline_layout_info.pSetLayouts = descriptorSetLayouts.data();
		pipeline_layout_info.pushConstantRangeCount = passCreationData->frame_graph_node.gpuPipelineLayout.pushConstantRanges.size();
		pipeline_layout_info.pPushConstantRanges = passCreationData->frame_graph_node.gpuPipelineLayout.pushConstantRanges.data();

		if( vkCreatePipelineLayout( g_vk.device, &pipeline_layout_info, nullptr, &o_technique->pipelineLayout ) != VK_SUCCESS ) {
			throw std::runtime_error( "failed to create pipeline layout!" );
		}

		//TODO: temp hack to get a relative size;
		GpuPipelineState pipelineState = passCreationData->frame_graph_node.gpuPipelineState;
		if( pipelineState.framebufferExtent.width == 0 && pipelineState.framebufferExtent.height == 0 )
			pipelineState.framebufferExtent = renderpass->outputFrameBuffer[0].extent;

		CreatePipeline( pipelineState,
			renderpass->vk_renderpass,
			o_technique->pipelineLayout,
			&o_technique->pipeline );
	}

	void CreateTechniques( FG::FrameGraph* frameGraph, VkDescriptorPool descriptorPool, std::array< GpuInputData, SIMULTANEOUS_FRAMES>& inputBuffers )
	{
		for( uint32_t i = 0; i < frameGraph->imp->_render_passes_count; ++i )
		{
			CreateTechnique( frameGraph, descriptorPool, &frameGraph->imp->_render_passes[i], &frameGraph->imp->creationData.renderPasses[i], &frameGraph->imp->_techniques[i], inputBuffers );
			++frameGraph->imp->_techniques_count;
		}
	}

	static void UpdateDescriptorSet( const FG::FrameGraph* frameGraph, const GpuInputData* inputData, const TechniqueDescriptorSetDesc* descriptorSetDesc, VkDescriptorSet* descriptorSet )
	{
		//Update descriptor sets
		assert( descriptorSetDesc->dataBindings.size() <= MAX_DATA_ENTRIES );
		WriteDescriptor writeDescriptors[8];
		uint32_t writeDescriptorsCount = 0;
		WriteDescriptorSet writeDescriptorSet = { writeDescriptors, descriptorSetDesc->dataBindings.size() };

		VkDescriptorBufferInfo descriptorBuffersInfos[16];
		uint32_t descriptorBuffersInfosCount = 0;
		VkDescriptorImageInfo descriptorImagesInfos[16];
		uint32_t descriptorImagesInfosCount = 0;

		//Fill in buffer
		for( uint32_t dataBindingIndex = 0; dataBindingIndex < descriptorSetDesc->dataBindings.size(); ++dataBindingIndex )
		{
			const TechniqueDataBinding* dataBinding = &descriptorSetDesc->dataBindings[dataBindingIndex];
			const FG::TechniqueDataEntry* techniqueDataEntry = GetDataEntry( frameGraph, dataBinding->id );
			if( IsBufferType( techniqueDataEntry->descriptorType ) )//Buffers
			{
				uint32_t bufferStart = descriptorBuffersInfosCount;
				GpuBuffer* buffers = GetBuffer( inputData, dataBinding->id );
				uint32_t buffersCount = GetDataCount( inputData, dataBinding->id );
				assert( buffersCount <= techniqueDataEntry->count );
				for( uint32_t descriptorIndex = 0; descriptorIndex < buffersCount; ++descriptorIndex )
				{
					assert( descriptorBuffersInfosCount < 16 );
					descriptorBuffersInfos[descriptorBuffersInfosCount++] = { buffers[descriptorIndex].buffer, 0, VK_WHOLE_SIZE };
				}
				writeDescriptors[writeDescriptorsCount++] = { dataBinding->binding, buffersCount, DescriptorTypeToVkType( techniqueDataEntry->descriptorType, dataBinding->descriptorAccess ), &descriptorBuffersInfos[bufferStart], nullptr };
			}
			else if( techniqueDataEntry->descriptorType == eDescriptorType::IMAGE_SAMPLER ) // Combined image samplers
			{
				VkDescriptorImageInfo* images = GetImage( inputData, dataBinding->id );
				uint32_t bufferStart = descriptorImagesInfosCount;
				uint32_t buffersCount = GetDataCount( inputData, dataBinding->id );
				assert( buffersCount <= techniqueDataEntry->count );
				for( uint32_t descriptorIndex = 0; descriptorIndex < buffersCount; ++descriptorIndex )
				{
					assert( descriptorImagesInfosCount < 16 );
					//TODO: HACK shouldn't pass in this struct in the input buffer, should be some wrapper or something
					descriptorImagesInfos[descriptorImagesInfosCount++] = images[descriptorIndex];
				}
				writeDescriptors[writeDescriptorsCount++] = { dataBinding->binding, buffersCount, DescriptorTypeToVkType( techniqueDataEntry->descriptorType, dataBinding->descriptorAccess ), nullptr, &descriptorImagesInfos[bufferStart] };
			}
			else
			{
				//TODO: Other image types not yet implemented
				assert( true );
			}
		}

		UpdateDescriptorSets( 1, &writeDescriptorSet, descriptorSet );
	}

	void UpdateTechniqueDescriptorSets( const FG::FrameGraph* frameGraph, const std::array< GpuInputData, SIMULTANEOUS_FRAMES>& inputBuffers )
	{
		for( uint32_t i = 0; i < frameGraph->imp->_render_passes_count; ++i )
		{
			Technique* technique = &frameGraph->imp->_techniques[i];
			const FG::RenderPassCreationData* passCreationData = &frameGraph->imp->creationData.renderPasses[i];
			const TechniqueDescriptorSetDesc* passSet = passCreationData->frame_graph_node.passSet;
			const TechniqueDescriptorSetDesc* instanceSet = passCreationData->frame_graph_node.instanceSet;
			//Update descriptors
			if( passSet )
			{
				//TODO: not all of them need one for each simultaneous frames
				for( size_t i = 0; i < SIMULTANEOUS_FRAMES; ++i )
					UpdateDescriptorSet( frameGraph, &inputBuffers[i], passSet, &technique->renderPass_descriptor[i] );
			}
			if( instanceSet )
			{
				//TODO: not all of them need one for each simultaneous frames
				for( size_t i = 0; i < SIMULTANEOUS_FRAMES; ++i )
					UpdateDescriptorSet( frameGraph, &inputBuffers[i], instanceSet, &technique->instance_descriptor[i] );
			}
		}
	}
}