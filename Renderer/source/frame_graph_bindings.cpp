#include "frame_graph_bindings.h"
#include "frame_graph_common_internal.h"
#include "gfx_heaps_batched_allocator.h"

#include "descriptors.h"
#include "material.h"

namespace FG
{
	GfxImage dummyImage;
	
	static const FG::DataEntry* GetDataEntry( const FG::FrameGraph* frameGraph, uint32_t entryId )
	{
		const FG::DataEntry* dataEntry = &frameGraph->imp->creationData.resources[entryId];
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
		default:
			throw std::runtime_error( "Unknown descriptor type" );
		}
	}

	static VkDescriptorSetLayoutBinding CreateSetLayoutBinding( const GfxDataBinding* dataBinding, const FG::DataEntry* dataEntry )
	{
		VkDescriptorSetLayoutBinding layoutBinding;
		layoutBinding.binding = dataBinding->binding;
		layoutBinding.descriptorCount = dataEntry->count;
		layoutBinding.descriptorType = DescriptorTypeToVkType( dataEntry->descriptorType, dataBinding->descriptorAccess );
		layoutBinding.stageFlags = dataBinding->stageFlags;
		layoutBinding.pImmutableSamplers = nullptr;
		return layoutBinding;
	}

	static void CreateDescriptorSetLayout( const FG::FrameGraph* frameGraph, const GfxDescriptorSetDesc * desc, VkDescriptorSetLayout * o_setLayout )
	{
		std::array<VkDescriptorSetLayoutBinding, 8> tempBindings;
		uint32_t count = 0;

		for( uint32_t i = 0; i < desc->dataBindings.size(); ++i, ++count )
		{
			const GfxDataBinding* dataBinding = &desc->dataBindings[i];
			const FG::DataEntry* dataEntry = GetDataEntry( frameGraph, dataBinding->id );

			tempBindings[count] = CreateSetLayoutBinding( dataBinding, dataEntry );
		}

		CreateDesciptorSetLayout( tempBindings.data(), count, o_setLayout );
	}

	static void CreateDescriptorSet( VkDescriptorSetLayout descriptorSetLayout, VkDescriptorPool descriptorPool, VkDescriptorSet* o_descriptorSet )
	{
		CreateDescriptorSets( descriptorPool, 1, &descriptorSetLayout, o_descriptorSet );		
	}

	static void CreatePerFrameBuffer( const FG::FrameGraph* frameGraph, const FG::DataEntry* techniqueDataEntry, const GfxDataBinding* dataBinding, PerFrameBuffer* o_buffer )
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

	static void SetOrCreateDataIfNeeded( FG::FrameGraph* frameGraph, std::array< GpuInputData, SIMULTANEOUS_FRAMES>* inputBuffers, const GfxDescriptorSetDesc* descriptorSetDesc )
	{
		for( uint32_t i = 0; i < descriptorSetDesc->dataBindings.size(); ++i )
		{
			const GfxDataBinding* dataBinding = &descriptorSetDesc->dataBindings[i];
			const FG::DataEntry* dataEntry = GetDataEntry( frameGraph, dataBinding->id );

			if( dataEntry->flags & eDataEntryFlags::EXTERNAL )
				continue;

			if( IsBufferType( dataEntry->descriptorType ) )
			{
				PerFrameBuffer* buffer = &frameGraph->imp->allbuffers[dataEntry->id];
				if( buffer->gfx_mem_alloc.memory == VK_NULL_HANDLE )
				{
					CreatePerFrameBuffer( frameGraph, dataEntry, dataBinding, buffer );
					for( size_t i = 0; i < SIMULTANEOUS_FRAMES; ++i )
						SetBuffers( &(*inputBuffers)[i], dataEntry->id, &buffer->buffers[i], 1 );
				}
			}
			else
			{
				const GfxImage* image = frameGraph->imp->GetImage( dataEntry->id ); //TODO: GetResource( (uint32_t) id ) --------------------
				GfxImageSamplerCombined* imageInfo = &frameGraph->imp->allImages[dataEntry->id];
				if( imageInfo->image == nullptr )
				{
					*imageInfo = { const_cast< GfxImage* >(image), GetSampler( dataEntry->sampler ) };
					for( size_t i = 0; i < SIMULTANEOUS_FRAMES; ++i )
						SetImages( &(*inputBuffers)[i], dataEntry->id, imageInfo, 1 );
				}
			}
		}
	}

	static Technique CreateTechnique( FG::FrameGraph* frameGraph, VkDescriptorPool descriptorPool, const RenderPass* renderpass, const FG::RenderPassCreationData* passCreationData )
	{
		Technique technique;
		//Create descriptors
		for( const GfxDescriptorSetDesc& setDesc : passCreationData->frame_graph_node.descriptorSets )
		{
			VkDescriptorSetLayout layout;
			CreateDescriptorSetLayout( frameGraph, &setDesc, &layout );

			//TODO: not all of them need one for each simultaneous frames
			GfxDescriptorSetBinding setBinding;
			setBinding.desc = setDesc;
			setBinding.hw_layout = layout;
			setBinding.isValid = true;

			for( size_t i = 0; i < SIMULTANEOUS_FRAMES; ++i )
				CreateDescriptorSet( layout, descriptorPool, &setBinding.hw_descriptorSets[i]);

			technique.descriptor_sets[setDesc.id] = setBinding;
		}
		technique.parentDescriptorPool = descriptorPool;

		//Create pipeline layout
		//TODO: this won't work if there's one binding point with no set. As the location in the array denote the set's binding point
		std::vector< VkDescriptorSetLayout > descriptorSetLayouts;
		for( const GfxDescriptorSetDesc& setDesc : passCreationData->frame_graph_node.descriptorSets )
			descriptorSetLayouts.push_back( technique.descriptor_sets[setDesc.id].hw_layout );

		VkPipelineLayoutCreateInfo pipeline_layout_info = {};
		pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_info.setLayoutCount = descriptorSetLayouts.size();
		pipeline_layout_info.pSetLayouts = descriptorSetLayouts.data();
		pipeline_layout_info.pushConstantRangeCount = passCreationData->frame_graph_node.gpuPipelineLayout.pushConstantRanges.size();
		pipeline_layout_info.pPushConstantRanges = passCreationData->frame_graph_node.gpuPipelineLayout.pushConstantRanges.data();

		if( vkCreatePipelineLayout( g_vk.device, &pipeline_layout_info, nullptr, &technique.pipelineLayout ) != VK_SUCCESS ) {
			throw std::runtime_error( "failed to create pipeline layout!" );
		}
				
		GpuPipelineState pipelineState = passCreationData->frame_graph_node.gpuPipelineState;
		VkExtent2D viewportSize = renderpass->outputFrameBuffer[0].extent;//TODO: temp hack to get a relative size;

		CreatePipeline( pipelineState,
			viewportSize,
			renderpass->vk_renderpass,
			technique.pipelineLayout,
			&technique.pipeline );

		return technique;
	}

	void CreateTechniques( FG::FrameGraph* frameGraph, VkDescriptorPool descriptorPool )
	{
		for( uint32_t i = 0; i < frameGraph->imp->_render_passes_count; ++i )
		{
			frameGraph->imp->_techniques[i] = CreateTechnique( frameGraph, descriptorPool, &frameGraph->imp->_render_passes[i], &frameGraph->imp->creationData.renderPasses[i] );
			++frameGraph->imp->_techniques_count;
		}
	}

	void SetupInputBuffers( FG::FrameGraph* frameGraph, std::array< GpuInputData, SIMULTANEOUS_FRAMES>& inputBuffers )
	{
		for( uint32_t i = 0; i < frameGraph->imp->_render_passes_count; ++i )
		{
			const FG::RenderPassCreationData* passCreationData = &frameGraph->imp->creationData.renderPasses[i];

			for( const GfxDescriptorSetDesc& set : passCreationData->frame_graph_node.descriptorSets )
				SetOrCreateDataIfNeeded( frameGraph, &inputBuffers, &set );
		}
	}

	static void UpdateDescriptorSet( const FG::FrameGraph* frameGraph, const GpuInputData* inputData, const GfxDescriptorSetDesc* descriptorSetDesc, VkDescriptorSet* descriptorSet )
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
			const GfxDataBinding* dataBinding = &descriptorSetDesc->dataBindings[dataBindingIndex];
			const FG::DataEntry* techniqueDataEntry = GetDataEntry( frameGraph, dataBinding->id );
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
			else if( techniqueDataEntry->descriptorType == eDescriptorType::IMAGE_SAMPLER || techniqueDataEntry->descriptorType == eDescriptorType::IMAGE )
			{
				GfxImageSamplerCombined* images = GetImage( inputData, dataBinding->id );
				uint32_t bufferStart = descriptorImagesInfosCount;
				uint32_t buffersCount = GetDataCount( inputData, dataBinding->id );
				assert( buffersCount <= techniqueDataEntry->count );
				for( uint32_t descriptorIndex = 0; descriptorIndex < buffersCount; ++descriptorIndex )
				{
					assert( descriptorImagesInfosCount < 16 );
					descriptorImagesInfos[descriptorImagesInfosCount++] = { images[descriptorIndex].sampler, images[descriptorIndex].image->imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
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

	static void FillWithDummyDescriptors( const FG::FrameGraph* frameGraph, const GpuInputData* inputData, const GfxDescriptorSetDesc& descriptorSetDesc, VkDescriptorSet* descriptorSet )
	{
		//Update descriptor sets
		assert( descriptorSetDesc.dataBindings.size() <= MAX_DATA_ENTRIES );
		WriteDescriptor writeDescriptors[8];
		uint32_t writeDescriptorsCount = 0;
		WriteDescriptorSet writeDescriptorSet = { writeDescriptors, descriptorSetDesc.dataBindings.size() };

		VkDescriptorBufferInfo descriptorBuffersInfos[16];
		uint32_t descriptorBuffersInfosCount = 0;
		VkDescriptorImageInfo descriptorImagesInfos[16];
		uint32_t descriptorImagesInfosCount = 0;

		//Fill in buffer
		for( uint32_t dataBindingIndex = 0; dataBindingIndex < descriptorSetDesc.dataBindings.size(); ++dataBindingIndex )
		{
			const GfxDataBinding* dataBinding = &descriptorSetDesc.dataBindings[dataBindingIndex];
			const FG::DataEntry* techniqueDataEntry = GetDataEntry( frameGraph, dataBinding->id );
			if( IsBufferType( techniqueDataEntry->descriptorType ) )//Buffers
			{
			}
			else if( techniqueDataEntry->descriptorType == eDescriptorType::IMAGE_SAMPLER ) // Combined image samplers
			{
				uint32_t bufferStart = descriptorImagesInfosCount;
				for( uint32_t descriptorIndex = 0; descriptorIndex < techniqueDataEntry->count; ++descriptorIndex )
				{
					assert( descriptorImagesInfosCount < 16 );
					//TODO: HACK shouldn't pass in this struct in the input buffer, should be some wrapper or something
					descriptorImagesInfos[descriptorImagesInfosCount++] = { GetSampler(eSamplers::Trilinear), dummyImage.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
				}
				writeDescriptors[writeDescriptorsCount++] = { dataBinding->binding, techniqueDataEntry->count, DescriptorTypeToVkType( techniqueDataEntry->descriptorType, dataBinding->descriptorAccess ), nullptr, &descriptorImagesInfos[bufferStart] };
			}
			else
			{
				//TODO: Other image types not yet implemented
				assert( true );
			}
		}
		writeDescriptorSet.count = writeDescriptorsCount;
		UpdateDescriptorSets( 1, &writeDescriptorSet, descriptorSet );
	}

	void UpdateTechniqueDescriptorSets( const FG::FrameGraph* frameGraph, const std::array< GpuInputData, SIMULTANEOUS_FRAMES>& inputBuffers )
	{
		//TODO: dsetroy this dummy image and find a better system to bind null or remove the warning for unbound descriptors
		if( dummyImage.gfx_mem_alloc.memory == VK_NULL_HANDLE )
		{
			GfxHeaps_CommitedResourceAllocator allocator = {};
			allocator.Prepare();
			CreateSolidColorImage( glm::vec4( 0, 0, 0, 0 ), &dummyImage, &allocator );
			allocator.Commit();
		}

		for( uint32_t i = 0; i < frameGraph->imp->_render_passes_count; ++i )
		{
			Technique& technique = frameGraph->imp->_techniques[i];
			const FG::RenderPassCreationData* passCreationData = &frameGraph->imp->creationData.renderPasses[i];

			for( const GfxDescriptorSetDesc& setDesc : passCreationData->frame_graph_node.descriptorSets )
			{
				//TODO: not all of them need one for each simultaneous frames
				for( size_t i = 0; i < SIMULTANEOUS_FRAMES; ++i )
				{
					VkDescriptorSet descriptorSet = technique.descriptor_sets[setDesc.id].hw_descriptorSets[i];
					FillWithDummyDescriptors( frameGraph, &inputBuffers[i], setDesc, &descriptorSet );
					UpdateDescriptorSet( frameGraph, &inputBuffers[i], &setDesc, &descriptorSet );
				}
			}
		}
	}
}