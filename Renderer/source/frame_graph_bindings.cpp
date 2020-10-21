#include "frame_graph_bindings.h"
#include "frame_graph_common_internal.h"
#include "gfx_heaps_batched_allocator.h"

#include "vk_globals.h"
#include "material.h"
#include <stdexcept>

namespace FG
{	
	static const FG::DataEntry* GetDataEntry( const FG::FrameGraph* frameGraph, uint32_t entryId )
	{
		const FG::DataEntry* dataEntry = &frameGraph->imp->creationData.resources[entryId];
		assert( dataEntry->id == entryId );
		return dataEntry;
	}

	static GfxDescriptorTableLayoutBinding CreateDescriptorTableLayoutBinding( const GfxDataBinding* dataBinding, const FG::DataEntry* dataEntry )
	{
		return CreateDescriptorTableLayoutBinding( dataBinding->binding, dataBinding->stageFlags, dataEntry->descriptorType, dataBinding->descriptorAccess, dataEntry->count );
	}

	static void CreateDescriptorTableLayout( const FG::FrameGraph* frameGraph, const GfxDescriptorTableDesc * desc, GfxDescriptorTableLayout * o_tableLayout )
	{
		std::array<GfxDescriptorTableLayoutBinding, 8> tempBindings;
		uint32_t count = 0;

		for( uint32_t i = 0; i < desc->dataBindings.size(); ++i, ++count )
		{
			const GfxDataBinding* dataBinding = &desc->dataBindings[i];
			const FG::DataEntry* dataEntry = GetDataEntry( frameGraph, dataBinding->id );

			tempBindings[count] = CreateDescriptorTableLayoutBinding( dataBinding, dataEntry );
		}

		CreateDesciptorTableLayout( tempBindings.data(), count, o_tableLayout );
	}

	static void CreateDescriptorTable( GfxDescriptorTableLayout descriptorSetLayout, GfxDescriptorPool descriptorPool, GfxDescriptorTable* o_descriptorSet )
	{
		CreateDescriptorTables( descriptorPool, 1, &descriptorSetLayout, o_descriptorSet );
	}

	static void CreatePerFrameBuffer( const FG::FrameGraph* frameGraph, const FG::DataEntry* techniqueDataEntry, const GfxDataBinding* dataBinding, PerFrameBuffer* o_buffer )
	{
		GfxDeviceSize size;
		switch( techniqueDataEntry->descriptorType )
		{
		case eDescriptorType::BUFFER:
			size = techniqueDataEntry->resourceDesc.extent.width;
			break;
		case eDescriptorType::BUFFER_DYNAMIC:
			size = techniqueDataEntry->resourceDesc.extent.width * techniqueDataEntry->resourceDesc.extent.height;
		}

		//TODO: could have to change VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT if we write (store). Will have to check all bindings to know.
		CreatePerFrameBuffer( size, GFX_BUFFER_USAGE_UNIFORM_BUFFER_BIT, GFX_MEMORY_PROPERTY_HOST_VISIBLE_BIT | GFX_MEMORY_PROPERTY_HOST_COHERENT_BIT, o_buffer );
	}

	static void SetOrCreateDataIfNeeded( FG::FrameGraph* frameGraph, std::array< GpuInputData, SIMULTANEOUS_FRAMES>* inputBuffers, const GfxDescriptorTableDesc* descriptorSetDesc )
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
				if( !IsValid( buffer->gfx_mem_alloc ) )
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

	static Technique CreateTechnique( FG::FrameGraph* frameGraph, GfxDescriptorPool descriptorPool, const RenderPass* renderpass, const FG::RenderPassCreationData* passCreationData )
	{
		Technique technique;

		assert( passCreationData->frame_graph_node.descriptorSets.size() < 8 );
		GfxDescriptorTableLayout layouts [8];
		for(  uint32_t i = 0; i < passCreationData->frame_graph_node.descriptorSets.size(); ++i )
		{
			const GfxDescriptorTableDesc& setDesc = passCreationData->frame_graph_node.descriptorSets[i];
			GfxDescriptorTableLayout& layout = layouts[i];
			CreateDescriptorTableLayout( frameGraph, &setDesc, &layout );

			//TODO: not all of them need one for each simultaneous frames
			GfxDescriptorSetBinding setBinding;
			setBinding.desc = setDesc;
			setBinding.hw_layout = layout;
			setBinding.isValid = true;

			for( size_t i = 0; i < SIMULTANEOUS_FRAMES; ++i )
				CreateDescriptorTable( layout, descriptorPool, &setBinding.hw_descriptorSets[i]);

			technique.descriptor_sets[setDesc.id] = setBinding;
		}
		technique.parentDescriptorPool = descriptorPool;

		CreateGfxPipelineLayout( passCreationData->frame_graph_node.descriptorSets.data(), layouts, passCreationData->frame_graph_node.descriptorSets.size(),
			passCreationData->frame_graph_node.gpuPipelineLayout.RootConstantRanges.data(), passCreationData->frame_graph_node.gpuPipelineLayout.RootConstantRanges.size(),
			&technique.pipelineLayout );
				
		CreatePipeline( passCreationData->frame_graph_node.gpuPipelineStateDesc,
			*renderpass,
			technique.pipelineLayout,
			&technique.pipeline );

		return technique;
	}

	void CreateTechniques( FG::FrameGraph* frameGraph, GfxDescriptorPool descriptorPool )
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

			for( const GfxDescriptorTableDesc& set : passCreationData->frame_graph_node.descriptorSets )
				SetOrCreateDataIfNeeded( frameGraph, &inputBuffers, &set );
		}
	}

	static void UpdateDescriptorTable( const FG::FrameGraph* frameGraph, const GpuInputData* inputData, const GfxDescriptorTableDesc* descriptorSetDesc, GfxDescriptorTable* descriptorTable )
	{
		assert( descriptorSetDesc->dataBindings.size() <= MAX_DATA_ENTRIES );
		BatchDescriptorsUpdater batchDescriptorsUpdater;

		//Fill in buffer
		for( uint32_t dataBindingIndex = 0; dataBindingIndex < descriptorSetDesc->dataBindings.size(); ++dataBindingIndex )
		{
			const GfxDataBinding* dataBinding = &descriptorSetDesc->dataBindings[dataBindingIndex];
			const FG::DataEntry* techniqueDataEntry = GetDataEntry( frameGraph, dataBinding->id );
			uint32_t buffersCount = GetDataCount( inputData, dataBinding->id );
			assert( buffersCount <= techniqueDataEntry->count );

			batchDescriptorsUpdater.AddBinding( GetData( inputData, dataBinding->id ), buffersCount, dataBinding->binding, techniqueDataEntry->descriptorType, dataBinding->descriptorAccess );
		}

		batchDescriptorsUpdater.Submit( *descriptorTable );
	}

	static void FillWithDummyDescriptors( const FG::FrameGraph* frameGraph, const GpuInputData* inputData, const GfxDescriptorTableDesc& descriptorSetDesc, GfxDescriptorTable* descriptorTable )
	{
		assert( descriptorSetDesc.dataBindings.size() <= MAX_DATA_ENTRIES );
		BatchDescriptorsUpdater batchDescriptorsUpdater;

		const GfxImage* dummyImage = GetDummyImage();
		const GfxImageSamplerCombined combinedDummyImage = { const_cast< GfxImage*>(dummyImage), GetSampler( eSamplers::Trilinear ) };
		constexpr uint32_t maxDescriptors = 16;
		GfxImageSamplerCombined dummyDescriptors[maxDescriptors];
		for( uint32_t i = 0; i < maxDescriptors; ++i )
			dummyDescriptors[i] = combinedDummyImage;

		//Fill in buffer
		for( uint32_t dataBindingIndex = 0; dataBindingIndex < descriptorSetDesc.dataBindings.size(); ++dataBindingIndex )
		{
			const GfxDataBinding* dataBinding = &descriptorSetDesc.dataBindings[dataBindingIndex];
			const FG::DataEntry* techniqueDataEntry = GetDataEntry( frameGraph, dataBinding->id );
			uint32_t buffersCount = GetDataCount( inputData, dataBinding->id );
			if( IsBufferType( techniqueDataEntry->descriptorType ) )//Buffers
			{
			}
			else if( techniqueDataEntry->descriptorType == eDescriptorType::IMAGE_SAMPLER ) // Combined image samplers
			{
				assert( techniqueDataEntry->count <= maxDescriptors );
				batchDescriptorsUpdater.AddImagesBinding( dummyDescriptors, techniqueDataEntry->count, dataBinding->binding, techniqueDataEntry->descriptorType, dataBinding->descriptorAccess );
			}
			else
			{
				//TODO: Other image types not yet implemented
				assert( true );
			}
		}

		batchDescriptorsUpdater.Submit( *descriptorTable );
	}

	void UpdateTechniqueDescriptorSets( const FG::FrameGraph* frameGraph, const std::array< GpuInputData, SIMULTANEOUS_FRAMES>& inputBuffers )
	{
		for( uint32_t i = 0; i < frameGraph->imp->_render_passes_count; ++i )
		{
			Technique& technique = frameGraph->imp->_techniques[i];
			const FG::RenderPassCreationData* passCreationData = &frameGraph->imp->creationData.renderPasses[i];

			for( const GfxDescriptorTableDesc& setDesc : passCreationData->frame_graph_node.descriptorSets )
			{
				//TODO: not all of them need one for each simultaneous frames
				for( size_t i = 0; i < SIMULTANEOUS_FRAMES; ++i )
				{
					GfxDescriptorTable descriptorTable = technique.descriptor_sets[setDesc.id].hw_descriptorSets[i];
					FillWithDummyDescriptors( frameGraph, &inputBuffers[i], setDesc, &descriptorTable );
					UpdateDescriptorTable( frameGraph, &inputBuffers[i], &setDesc, &descriptorTable );
				}
			}
		}
	}
}