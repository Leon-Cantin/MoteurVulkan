#include "vk_globals.h"

#include <cassert>


//TODO I could infer what type to use with "type" (image, buffer, etc ...)"
void BatchDescriptorsUpdater::AddImagesBinding( const GfxImageSamplerCombined* images, uint32_t count, uint32_t binding, eDescriptorType type, eDescriptorAccess access )
{
	uint32_t bufferStart = descriptorImagesInfosCount;
	for( uint32_t descriptorIndex = 0; descriptorIndex < count; ++descriptorIndex )
	{
		assert( descriptorImagesInfosCount < MAX_DESCRIPTOR_PER_UPDATE );
		descriptorImagesInfos[descriptorImagesInfosCount++] = { images[descriptorIndex].sampler, images[descriptorIndex].image->imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
	}
	//TODO: type could probably be infered
	writeDescriptors[writeDescriptorsCount++] = { binding, count, DescriptorTypeToVkType( type, access ), nullptr, &descriptorImagesInfos[bufferStart] };
}

void BatchDescriptorsUpdater::AddBuffersBinding( const GpuBuffer* buffers, uint32_t count, uint32_t binding, eDescriptorType type, eDescriptorAccess access )
{
	uint32_t bufferStart = descriptorBuffersInfosCount;
	for( uint32_t descriptorIndex = 0; descriptorIndex < count; ++descriptorIndex )
	{
		assert( descriptorBuffersInfosCount < MAX_DESCRIPTOR_PER_UPDATE );
		descriptorBuffersInfos[descriptorBuffersInfosCount++] = { buffers[descriptorIndex].buffer, 0, VK_WHOLE_SIZE };
	}
	writeDescriptors[writeDescriptorsCount++] = { binding, count, DescriptorTypeToVkType( type, access ), &descriptorBuffersInfos[bufferStart], nullptr };
}

void BatchDescriptorsUpdater::AddBinding( const void* data, uint32_t count, uint32_t binding, eDescriptorType type, eDescriptorAccess access )
{
	if( IsBufferType( type ) )//Buffers
	{
		AddBuffersBinding( static_cast< const GpuBuffer* >(data), count, binding, type, access );
	}
	else if( type == eDescriptorType::IMAGE_SAMPLER || type == eDescriptorType::IMAGE )
	{
		AddImagesBinding( static_cast< const GfxImageSamplerCombined* >(data), count, binding, type, access );
	}
	else
	{
		//TODO: Other image types not yet implemented
		assert( true );
	}
}

void BatchDescriptorsUpdater::Submit( GfxDescriptorTable descriptorSet )
{
	WriteDescriptorTable writeDescriptorSet = { writeDescriptors, writeDescriptorsCount };
	UpdateDescriptorTables( 1, &writeDescriptorSet, &descriptorSet );
}
