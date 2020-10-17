#pragma once

#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#elif defined __linux__
#define VK_USE_PLATFORM_XCB_KHR
#endif

#include <vulkan/vulkan.h>
//TODO: find a way of getting rid of those undef
#undef max
#undef min

struct GpuInstance
{
	VkInstance instance;
	bool validationLayerEnabled;
};

typedef VkSurfaceKHR DisplaySurface;

typedef VkPhysicalDevice PhysicalDevice;

typedef VkDeviceSize GfxDeviceSize;

struct Queue
{
	VkQueue queue = VK_NULL_HANDLE;
	uint32_t queueFamilyIndex;
};

struct Device
{
	VkDevice device = VK_NULL_HANDLE;
	Queue graphics_queue;
	Queue present_queue;
	Queue compute_queue;
	Queue transfer_queue;
};

enum class GfxFormat
{
	UNDEFINED = VK_FORMAT_UNDEFINED,
	R8_UNORM = VK_FORMAT_R8_UNORM,
	R8_UINT = VK_FORMAT_R8_UINT,
	R8G8_UNORM = VK_FORMAT_R8G8_UNORM,
	R8G8_UINT = VK_FORMAT_R8G8_UINT,
	R8G8B8_UNORM = VK_FORMAT_R8G8B8_UNORM,
	R8G8B8_UINT = VK_FORMAT_R8G8B8_UINT,
	B8G8R8_UNORM = VK_FORMAT_B8G8R8_UNORM,
	B8G8R8_UINT = VK_FORMAT_B8G8R8_UINT,
	R8G8B8A8_UNORM = VK_FORMAT_R8G8B8A8_UNORM,
	R8G8B8A8_UINT = VK_FORMAT_R8G8B8A8_UINT,
	B8G8R8A8_UNORM = VK_FORMAT_B8G8R8A8_UNORM,
	B8G8R8A8_UINT = VK_FORMAT_B8G8R8A8_UINT,
	D32_SFLOAT = VK_FORMAT_D32_SFLOAT,
	S8_UINT = VK_FORMAT_S8_UINT,
};

enum class GfxLayout
{
	UNDEFINED,
	GENERAL,
	COLOR,
	DEPTH_STENCIL,
	TRANSFER,
	PRESENT,
};

enum class GfxAccess
{
	READ,
	WRITE,
};

enum class GfxLoadOp
{
	DONT_CARE = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
	LOAD = VK_ATTACHMENT_LOAD_OP_LOAD,
	CLEAR = VK_ATTACHMENT_LOAD_OP_CLEAR,
};

enum class GfxFilter
{
	NEAREST = VK_FILTER_NEAREST,
	LINEAR = VK_FILTER_LINEAR,
};

typedef VkFlags GfxFlags;
typedef GfxFlags GfxMemoryPropertyFlags;

enum GfxMemoryPropertyBit : GfxMemoryPropertyFlags {
	DEVICE_LOCAL = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
	HOST_VISIBLE = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
	HOST_COHERENT  = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
};

typedef GfxFlags GfxImageUsageFlags;

enum GfxImageUsageFlagBits : GfxImageUsageFlags {
	TRANSFER_SRC = VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
	TRANSFER_DST = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
	SAMPLED = VK_IMAGE_USAGE_SAMPLED_BIT,
	STORAGE = VK_IMAGE_USAGE_STORAGE_BIT,
	COLOR_ATTACHMENT = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
	DEPTH_STENCIL_ATTACHMENT = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
	TRANSIENT_ATTACHMENT = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
	INPUT_ATTACHMENT = VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
};

typedef GfxFlags GfxImageAspectFlags;

enum GfxImageAspectFlagBits : GfxImageAspectFlags {
	COLOR = VK_IMAGE_ASPECT_COLOR_BIT,
	DEPTH = VK_IMAGE_ASPECT_DEPTH_BIT,
	STENCIL = VK_IMAGE_ASPECT_STENCIL_BIT,
};

typedef GfxFlags GfxBufferUsageFlags;

enum GfxBufferUsageFlagBits : GfxBufferUsageFlags {
	TRANSFER_SRC_BUFFER = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
	TRANSFER_DST_BUFFER = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	UNIFORM_TEXEL_BUFFER = VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT,
	STORAGE_TEXEL_BUFFER = VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT,
	UNIFORM_BUFFER = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
	STORAGE_BUFFER = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
	INDEX_BUFFER = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
	VERTEX_BUFFER = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
	INDIRECT_BUFFER = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
};

typedef VkImageView GfxImageView;
typedef VkImage GfxApiImage;
typedef VkBuffer GfxApiBuffer;

inline VkFormat ToVkFormat( GfxFormat gfxFormat )
{
	return static_cast< VkFormat >(gfxFormat);
};

inline GfxFormat ToGfxFormat( VkFormat gfxFormat )
{
	return static_cast< GfxFormat >(gfxFormat);
};

VkImageLayout ConvertToVkImageLayout( GfxLayout layout, GfxAccess access );
VkAttachmentLoadOp ConvertVkLoadOp( GfxLoadOp loadOp );

void GfxImageBarrier( VkCommandBuffer commandBuffer, GfxApiImage image, GfxLayout oldLayout, GfxAccess oldAccess, GfxLayout newLayout, GfxAccess newAccess, uint32_t baseMipLevel, uint32_t mipCount, uint32_t baseLayer, uint32_t layerCount );
void GfxImageBarrier( VkCommandBuffer commandBuffer, GfxApiImage image, GfxLayout oldLayout, GfxAccess oldAccess, GfxLayout newLayout, GfxAccess newAccess );

void CmdBlitImage( VkCommandBuffer commandBuffer, GfxApiImage srcImage, int32_t srcX1, int32_t srcY1, int32_t srcZ1, int32_t srcX2, int32_t srcSY2, int32_t srcZ2, uint32_t srcMipLevel,
	GfxApiImage dstImage, int32_t dstX1, int32_t dstY1, int32_t dstZ1, int32_t dstX2, int32_t dstY2, int32_t dstZ2, uint32_t dstMipLevel, GfxFilter filter );

typedef VkMemoryRequirements GfxMemoryRequirements;
typedef uint32_t GfxMemoryTypeFilter;
typedef uint32_t GfxMemoryType;

inline GfxDeviceSize GetSize( const VkMemoryRequirements& memoryRequirement )
{
	return memoryRequirement.size;
}

inline GfxDeviceSize GetAlignment( const VkMemoryRequirements& memoryRequirement )
{
	return memoryRequirement.alignment;
}

inline GfxMemoryTypeFilter GetMemoryTypeFilter( const VkMemoryRequirements& memoryRequirement )
{
	return memoryRequirement.memoryTypeBits;
}

GfxMemoryRequirements GetImageMemoryRequirement( GfxApiImage image );
GfxMemoryRequirements GetBufferMemoryRequirement( GfxApiBuffer buffer );

typedef VkSurfaceFormatKHR GfxSurfaceFormat;

inline GfxFormat GetFormat( const GfxSurfaceFormat& surfaceFormat )
{
	return ToGfxFormat( surfaceFormat.format );
}

#define GFX_REMAINING_MIP_LEVELS VK_REMAINING_MIP_LEVELS
#define GFX_REMAINING_ARRAY_LAYERS VK_REMAINING_ARRAY_LAYERS

struct Vk_Globals {
	GpuInstance instance = {};
	PhysicalDevice physicalDevice = VK_NULL_HANDLE;
	Device device = {};
	VkCommandPool graphicsSingleUseCommandPool = VK_NULL_HANDLE;
	VkCommandPool graphicsCommandPool = VK_NULL_HANDLE;
	VkCommandPool computeCommandPool = VK_NULL_HANDLE;
	VkCommandPool transferCommandPool = VK_NULL_HANDLE;
};

extern Vk_Globals g_vk;

const int SIMULTANEOUS_FRAMES = 2;