#include "frame_graph_bindings.h"
#include "frame_graph_common_internal.h"
#include "gfx_heaps_batched_allocator.h"

#include "vk_globals.h"
#include "material.h"
#include <stdexcept>

namespace FG
{	
	static const FG::DataEntry* GetDataEntryFromId( const FG::FrameGraph* frameGraph, user_id_t user_id )
	{
		/*const FG::DataEntry* dataEntry = &frameGraph->imp->creationData.resources[entryId];
		assert( dataEntry->id == entryId );
		return dataEntry;*/
		const FG::DataEntry* dataEntry;
		for( uint32_t i = 0; i < frameGraph->imp->creationData.resources.size(); ++i )
			if( frameGraph->imp->creationData.resources[i].user_id == user_id )
				return &frameGraph->imp->creationData.resources[i];

		return nullptr;
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
			const FG::DataEntry* dataEntry = GetDataEntryFromId( frameGraph, dataBinding->id );

			tempBindings[count] = CreateDescriptorTableLayoutBinding( dataBinding, dataEntry );
		}

		CreateDesciptorTableLayout( tempBindings.data(), count, o_tableLayout );
	}

	static void CreateDescriptorTable( GfxDescriptorTableLayout descriptorSetLayout, GfxDescriptorPool descriptorPool, GfxDescriptorTable* o_descriptorSet )
	{
		CreateDescriptorTables( descriptorPool, 1, &descriptorSetLayout, o_descriptorSet );
	}

	static void SetOrCreateDataIfNeeded( FG::FrameGraph* frameGraph, std::array< GpuInputData, SIMULTANEOUS_FRAMES>* inputBuffers, const GfxDescriptorTableDesc* descriptorSetDesc )
	{
		for( uint32_t i = 0; i < descriptorSetDesc->dataBindings.size(); ++i )
		{
			const GfxDataBinding* dataBinding = &descriptorSetDesc->dataBindings[i];
			const FG::DataEntry* dataEntry = GetDataEntryFromId( frameGraph, dataBinding->id );

			if( dataEntry->flags & eDataEntryFlags::EXTERNAL )
				continue;

			if( IsBufferType( dataEntry->descriptorType ) )
			{
				for( size_t frameIndex = 0; frameIndex < SIMULTANEOUS_FRAMES; ++frameIndex )
				{
					const GpuBuffer* buffer = frameGraph->imp->GetBufferFromId( dataEntry->user_id, frameIndex );
					assert( IsValid( buffer->gpuMemory ) );
					SetBuffers( &(*inputBuffers)[frameIndex], dataEntry->user_id, const_cast<GpuBuffer*>(buffer), 1 );
					//CreatePerFrameBuffer( frameGraph, dataEntry, dataBinding, buffer );
				}
			}
			else
			{
				const GfxImage* image = frameGraph->imp->GetImageFromId( dataEntry->user_id ); //TODO: GetResource( (uint32_t) id ) --------------------
				GfxImageSamplerCombined* imageInfo = &frameGraph->imp->allImages[dataEntry->user_id];
				if( imageInfo->image == nullptr )
				{
					*imageInfo = { const_cast< GfxImage* >(image), GetSampler( dataEntry->sampler ) };
					for( size_t frameIndex = 0; frameIndex < SIMULTANEOUS_FRAMES; ++frameIndex )
						SetImages( &(*inputBuffers)[frameIndex], dataEntry->user_id, imageInfo, 1 );
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
			const FG::DataEntry* techniqueDataEntry = GetDataEntryFromId( frameGraph, dataBinding->id );
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
			const FG::DataEntry* techniqueDataEntry = GetDataEntryFromId( frameGraph, dataBinding->id );
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