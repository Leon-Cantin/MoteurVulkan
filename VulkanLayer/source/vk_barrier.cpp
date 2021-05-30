#include "vk_globals.h"
#include <cassert>

namespace R_HW
{
	VkAccessFlags ToVkAccess( GfxLayout layout, GfxAccess access )
	{
		switch( layout )
		{
		case GfxLayout::PRESENT:
		case GfxLayout::UNDEFINED:
			return 0;
		case GfxLayout::TRANSFER:
			return access == GfxAccess::READ ? VK_ACCESS_TRANSFER_READ_BIT : VK_ACCESS_TRANSFER_WRITE_BIT;
		case GfxLayout::COLOR:
			return access == GfxAccess::READ ? VK_ACCESS_SHADER_READ_BIT : VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; // Maybe read could use VK_ACCESS_COLOR_ATTACHMENT_READ_BIT and write VK_ACCESS_SHADER_WRITE_BIT
		default:
			assert( false );
			return VK_ACCESS_FLAG_BITS_MAX_ENUM;
		}
	}

	VkPipelineStageFlags ToPipelineStage( GfxLayout layout )
	{
		//TODO: muchos is missing here
		switch( layout )
		{
		case GfxLayout::PRESENT:
		case GfxLayout::UNDEFINED:
			return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		case GfxLayout::TRANSFER:
			return VK_PIPELINE_STAGE_TRANSFER_BIT;
		case GfxLayout::COLOR:
			return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; //TODO: Too much stuff at once
		default:
			assert( false );
			return VK_PIPELINE_STAGE_FLAG_BITS_MAX_ENUM;
		}
	}

	void GfxImageBarrier( GfxCommandBuffer commandBuffer, GfxApiImage image, GfxLayout oldLayout, GfxAccess oldAccess, GfxLayout newLayout, GfxAccess newAccess, uint32_t baseMipLevel, uint32_t mipCount, uint32_t baseLayer, uint32_t layerCount )
	{
		//TODO: should use the format to set aspect according to VK's documentation
		VkImageAspectFlags aspectFlag = VK_IMAGE_ASPECT_COLOR_BIT;
		if( oldLayout == GfxLayout::DEPTH_STENCIL || newLayout == GfxLayout::DEPTH_STENCIL )
			aspectFlag = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;

		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.image = image;

		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.subresourceRange.aspectMask = aspectFlag;

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

	void GfxImageBarrier( GfxCommandBuffer commandBuffer, GfxApiImage image, GfxLayout oldLayout, GfxAccess oldAccess, GfxLayout newLayout, GfxAccess newAccess )
	{
		GfxImageBarrier( commandBuffer, image, oldLayout, oldAccess, newLayout, newAccess, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS );
	}
}