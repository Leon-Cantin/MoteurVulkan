#include "vk_globals.h"
#include <cassert>

VkAccessFlags ToVkAccess( GfxLayout layout, GfxAccess access )
{
	switch( layout )
	{
	case GfxLayout::UNDEFINED:
		return 0;
	case GfxLayout::TRANSFER :
		return access == GfxAccess::READ ? VK_ACCESS_TRANSFER_READ_BIT : VK_ACCESS_TRANSFER_WRITE_BIT;
	case GfxLayout::COLOR:
		return access == GfxAccess::READ ? VK_ACCESS_SHADER_READ_BIT : VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; // Maybe read could use VK_ACCESS_COLOR_ATTACHMENT_READ_BIT and write VK_ACCESS_SHADER_WRITE_BIT
	default:
		assert( true );
		return VK_ACCESS_FLAG_BITS_MAX_ENUM;
	}
}

VkPipelineStageFlags ToPipelineStage( GfxLayout layout )
{
	//TODO: muchos is missing here
	switch( layout )
	{
	case GfxLayout::UNDEFINED:
		return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	case GfxLayout::TRANSFER:
		return VK_PIPELINE_STAGE_TRANSFER_BIT;
	case GfxLayout::COLOR:
		return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	default:
		assert( true );
		return VK_PIPELINE_STAGE_FLAG_BITS_MAX_ENUM;
	}
}

void GfxImageBarrier( VkCommandBuffer commandBuffer, GfxApiImage image, GfxLayout oldLayout, GfxAccess oldAccess, GfxLayout newLayout, GfxAccess newAccess, uint32_t baseMipLevel, uint32_t mipCount, uint32_t baseLayer, uint32_t layerCount )
{
	//TODO: use something like this to get the aspectMask
	/*if( newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL ) {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

		if( hasStencilComponent( format ) ) { I can know if the stencil is changing with some layouts having stencil read only only
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
	}
	else {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}*/

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = image;

	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // TODO: Not always going to be color

	barrier.subresourceRange.baseArrayLayer = baseLayer;
	barrier.subresourceRange.layerCount = layerCount;

	barrier.subresourceRange.baseMipLevel = baseMipLevel;
	barrier.subresourceRange.levelCount = mipCount;

	barrier.oldLayout = ConvertToVkImageLayout( oldLayout, oldAccess );
	barrier.newLayout = ConvertToVkImageLayout( newLayout, newAccess );
	barrier.srcAccessMask = ToVkAccess( oldLayout, oldAccess );
	barrier.dstAccessMask = ToVkAccess( newLayout, newAccess );

	vkCmdPipelineBarrier( commandBuffer,
		ToPipelineStage( oldLayout ), ToPipelineStage( newLayout ), 0,
		0, nullptr,
		0, nullptr,
		1, &barrier );
}

void GfxImageBarrier( VkCommandBuffer commandBuffer, GfxApiImage image, GfxLayout oldLayout, GfxAccess oldAccess, GfxLayout newLayout, GfxAccess newAccess )
{
	GfxImageBarrier( commandBuffer, image, oldLayout, oldAccess, newLayout, newAccess, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS );
}