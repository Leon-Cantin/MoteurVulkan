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

const int SIMULTANEOUS_FRAMES = 2;

#include <vector> //TODO: remove this, used in some structures

struct GpuInstance
{
	VkInstance instance;
	bool validationLayerEnabled;
};

typedef VkSurfaceKHR DisplaySurface;

typedef VkPhysicalDevice PhysicalDevice;

typedef VkDeviceSize GfxDeviceSize;

typedef VkDevice GfxDevice;

typedef VkCommandPool GfxCommandPool;
typedef VkCommandBuffer GfxCommandBuffer;

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

enum class GfxMipFilter
{
	NEAREST = VK_SAMPLER_MIPMAP_MODE_NEAREST,
	LINEAR = VK_SAMPLER_MIPMAP_MODE_LINEAR,
};

enum class GfxSamplerAddressMode {
	REPEAT = VK_SAMPLER_ADDRESS_MODE_REPEAT,
	MIRRORED_REPEAT = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
	CLAMP_TO_EDGE = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
	CLAMP_TO_BORDER = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
	MIRROR_CLAMP_TO_EDGE = VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE,
};

enum class GfxCompareOp {
	NEVER = VK_COMPARE_OP_NEVER,
	LESS = VK_COMPARE_OP_LESS,
	EQUAL = VK_COMPARE_OP_EQUAL,
	LESS_OR_EQUAL = VK_COMPARE_OP_LESS_OR_EQUAL,
	GREATER = VK_COMPARE_OP_GREATER,
	NOT_EQUAL = VK_COMPARE_OP_NOT_EQUAL,
	GREATER_OR_EQUAL = VK_COMPARE_OP_GREATER_OR_EQUAL,
	ALWAYS = VK_COMPARE_OP_ALWAYS,
	NONE = VK_COMPARE_OP_MAX_ENUM,
};

typedef VkSampler GfxApiSampler;

bool CreateSampler( GfxFilter minFilter, GfxFilter magFilter, GfxMipFilter mipFilter, float anisotropy, GfxSamplerAddressMode samplerAddressMode, GfxCompareOp compareOp, GfxApiSampler* o_sampler );
void Destroy( GfxApiSampler* sampler );

typedef VkFlags GfxFlags;
typedef GfxFlags GfxMemoryPropertyFlags;

enum GfxMemoryPropertyBit : GfxMemoryPropertyFlags {
	GFX_MEMORY_PROPERTY_DEVICE_LOCAL_BIT = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
	GFX_MEMORY_PROPERTY_HOST_VISIBLE_BIT = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
	GFX_MEMORY_PROPERTY_HOST_COHERENT_BIT = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
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
	GFX_BUFFER_USAGE_TRANSFER_SRC_BIT = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
	GFX_BUFFER_USAGE_TRANSFER_DST_BIT = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	GFX_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT = VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT,
	GFX_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT = VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT,
	GFX_BUFFER_USAGE_UNIFORM_BUFFER_BIT = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
	GFX_BUFFER_USAGE_STORAGE_BUFFER_BIT = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
	GFX_BUFFER_USAGE_INDEX_BUFFER_BIT = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
	GFX_BUFFER_USAGE_VERTEX_BUFFER_BIT = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
	GFX_BUFFER_USAGE_INDIRECT_BUFFER_BIT = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
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

void GfxImageBarrier( GfxCommandBuffer commandBuffer, GfxApiImage image, GfxLayout oldLayout, GfxAccess oldAccess, GfxLayout newLayout, GfxAccess newAccess, uint32_t baseMipLevel, uint32_t mipCount, uint32_t baseLayer, uint32_t layerCount );
void GfxImageBarrier( GfxCommandBuffer commandBuffer, GfxApiImage image, GfxLayout oldLayout, GfxAccess oldAccess, GfxLayout newLayout, GfxAccess newAccess );

void CmdBlitImage( GfxCommandBuffer commandBuffer, GfxApiImage srcImage, int32_t srcX1, int32_t srcY1, int32_t srcZ1, int32_t srcX2, int32_t srcSY2, int32_t srcZ2, uint32_t srcMipLevel,
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

enum class GfxIndexType
{
	UINT16 = VK_INDEX_TYPE_UINT16,
    UINT32 = VK_INDEX_TYPE_UINT32,
	UNKNOWN = VK_INDEX_TYPE_MAX_ENUM,
};

inline VkIndexType ToVkIndexType( GfxIndexType gfxIndexType )
{
	return static_cast<VkIndexType>( gfxIndexType );
}

typedef uint8_t VIDataType;

typedef VkPipelineStageFlags GfxPipelineStageFlag;

enum GfxPipelineStageFlagBits : GfxPipelineStageFlag {
	GFX_PIPELINE_STAGE_TOP_OF_PIPE_BIT = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
	GFX_PIPELINE_STAGE_DRAW_INDIRECT_BIT = VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT,
	GFX_PIPELINE_STAGE_VERTEX_INPUT_BIT = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
	GFX_PIPELINE_STAGE_VERTEX_SHADER_BIT = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
	GFX_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT = VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT,
	GFX_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT = VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT,
	GFX_PIPELINE_STAGE_GEOMETRY_SHADER_BIT = VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT,
	GFX_PIPELINE_STAGE_FRAGMENT_SHADER_BIT = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
	GFX_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
	GFX_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
	GFX_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
	GFX_PIPELINE_STAGE_COMPUTE_SHADER_BIT = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
	GFX_PIPELINE_STAGE_TRANSFER_BIT = VK_PIPELINE_STAGE_TRANSFER_BIT,
	GFX_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
	GFX_PIPELINE_STAGE_HOST_BIT = VK_PIPELINE_STAGE_HOST_BIT,
	GFX_PIPELINE_STAGE_ALL_GRAPHICS_BIT = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
	GFX_PIPELINE_STAGE_ALL_COMMANDS_BIT = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
};

typedef VkQueryPool GfxTimeStampQueryPool;

GfxTimeStampQueryPool GfxApiCreateTimeStampsQueryPool( uint32_t queriesCount );
void GfxApiDestroyTimeStampsPool( GfxTimeStampQueryPool queryPool );
void GfxApiCmdResetTimeStamps( GfxCommandBuffer commandBuffer, GfxTimeStampQueryPool timeStampQueryPool, uint32_t firstQueryId, uint32_t count );
void GfxApiGetTimeStampResults( GfxTimeStampQueryPool timeStampQueryPool, uint32_t firstQueryId, uint32_t count, uint64_t* values );
void GfxApiCmdWriteTimestamp( GfxCommandBuffer commandBuffer, GfxTimeStampQueryPool timeStampQueryPool, GfxPipelineStageFlagBits stageBits, uint32_t queryId );

inline VkFilter ToVkFilter( GfxFilter filter )
{
	return static_cast< VkFilter >(filter);
}

inline VkSamplerMipmapMode ToVkMipFilter( GfxMipFilter mipFilter )
{
	return static_cast< VkSamplerMipmapMode >(mipFilter);
}

inline VkSamplerAddressMode ToVkSamplerAddressMode( GfxSamplerAddressMode samplerAddressMode )
{
	return static_cast< VkSamplerAddressMode >(samplerAddressMode);
}

inline VkCompareOp ToVkCompareOp( GfxCompareOp compareOp )
{
	return static_cast< VkCompareOp >(compareOp);
}

enum GfxShaderStageFlagBits 
{
	GFX_SHADER_STAGE_VERTEX_BIT = VK_SHADER_STAGE_VERTEX_BIT,
	GFX_SHADER_STAGE_TESSELLATION_CONTROL_BIT = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,
	GFX_SHADER_STAGE_TESSELLATION_EVALUATION_BIT = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
	GFX_SHADER_STAGE_GEOMETRY_BIT = VK_SHADER_STAGE_GEOMETRY_BIT,
	GFX_SHADER_STAGE_FRAGMENT_BIT = VK_SHADER_STAGE_FRAGMENT_BIT,
	GFX_SHADER_STAGE_COMPUTE_BIT = VK_SHADER_STAGE_COMPUTE_BIT,
	GFX_SHADER_STAGE_ALL_GRAPHICS = VK_SHADER_STAGE_ALL_GRAPHICS,
	GFX_SHADER_STAGE_ALL = VK_SHADER_STAGE_ALL,
};

inline VkShaderStageFlagBits ToVkShaderStageFlagBits( GfxShaderStageFlagBits flags )
{
	return static_cast< VkShaderStageFlagBits >(flags);
}

typedef GfxFlags GfxShaderStageFlags;

inline VkShaderStageFlags ToVkShaderStageFlags( GfxShaderStageFlags shaderStageFlag )
{
	return static_cast< VkShaderStageFlags >(shaderStageFlag);
}

enum eDescriptorType
{
	BUFFER = 0,
	BUFFER_DYNAMIC,
	IMAGE,
	SAMPLER,
	IMAGE_SAMPLER,
};

inline bool IsBufferType( eDescriptorType type )
{
	return type == eDescriptorType::BUFFER || type == eDescriptorType::BUFFER_DYNAMIC;
}

//TODO: Remove in favor of GfxAccess?
enum eDescriptorAccess
{
	READ = 0,
	WRITE,
};

VkDescriptorType DescriptorTypeToVkType( eDescriptorType type, eDescriptorAccess access );

typedef VkDescriptorSetLayoutBinding GfxDescriptorTableLayoutBinding;
typedef VkDescriptorSetLayout GfxDescriptorTableLayout;
typedef VkDescriptorSet GfxDescriptorTable;
typedef VkDescriptorSet GfxRootDescriptor;

enum class GfxPipelineBindPoint {
	GRAPHICS = VK_PIPELINE_BIND_POINT_GRAPHICS,
	COMPUTE = VK_PIPELINE_BIND_POINT_COMPUTE,
};

typedef VkPipelineLayout GfxPipelineLayout;
typedef uint32_t root_constant_t;

void CmdBindRootDescriptor( GfxCommandBuffer commandBuffer, GfxPipelineBindPoint pipelineBindPoint, GfxPipelineLayout pipelineLayout, uint32_t rootBindingPoint, GfxRootDescriptor rootDescriptor, uint32_t bufferOffset );

inline VkPipelineBindPoint ToVkPipelineBindPoint( GfxPipelineBindPoint pipelineBindPoint )
{
	return static_cast< VkPipelineBindPoint >(pipelineBindPoint);
}

GfxDescriptorTableLayoutBinding CreateDescriptorTableLayoutBinding( uint32_t descriptorBindingSlot, GfxShaderStageFlags descriptorStageFlags, eDescriptorType descriptorType, eDescriptorAccess descriptorAccess, uint32_t descriptorCount );

typedef VkDescriptorPool GfxDescriptorPool;

#define GFX_REMAINING_MIP_LEVELS VK_REMAINING_MIP_LEVELS
#define GFX_REMAINING_ARRAY_LAYERS VK_REMAINING_ARRAY_LAYERS

struct GfxRootConstantRange {
	GfxShaderStageFlags    stageFlags;
	uint32_t              offset;
	uint32_t              count;
};

struct GfxDataBinding
{
	uint32_t id;
	uint32_t binding;
	eDescriptorAccess descriptorAccess;
	GfxShaderStageFlags stageFlags;
};

struct GfxDescriptorTableDesc
{
	uint32_t id;
	std::vector<GfxDataBinding> dataBindings;
};

struct WriteDescriptor
{
	uint32_t dstBinding;
	uint32_t count;
	VkDescriptorType type;
	VkDescriptorBufferInfo* pBufferInfos;
	VkDescriptorImageInfo* pImageInfos;
};

struct WriteDescriptorTable
{
	WriteDescriptor* writeDescriptors;
	uint32_t count;
};

void CreateGfxPipelineLayout( const GfxDescriptorTableDesc* descriptorTablesDescs, const GfxDescriptorTableLayout* descriptorTableLayouts, uint32_t descriptorTablesDescsCount, const GfxRootConstantRange* rootConstantRanges, uint32_t rootConstantRangesCount, GfxPipelineLayout* o_pipelineLayout );

struct GfxMemAlloc
{
	VkDeviceMemory memory;
	GfxDeviceSize offset;
	GfxDeviceSize size;
	bool is_parent_pool;
};

struct GfxImage {
	//TODO: should split into the following. Also fix IsValid to check the image
	/*
	-image
		-image
		-format
		-extent
		-miplevels
	-view
	-alloc
	*/
	GfxApiImage image = VK_NULL_HANDLE;
	GfxImageView imageView = VK_NULL_HANDLE;
	GfxFormat format;
	VkExtent2D extent;
	uint32_t layers;
	uint32_t mipLevels;
	GfxMemAlloc gfx_mem_alloc;
};

struct GfxImageSamplerCombined
{
	GfxImage* image;
	VkSampler sampler = VK_NULL_HANDLE;
};

struct GpuBuffer
{
	GfxApiBuffer buffer;
	GfxMemAlloc gpuMemory;
};

class BatchDescriptorsUpdater
{
private:
	WriteDescriptor writeDescriptors[8];
	uint32_t writeDescriptorsCount = 0;

	VkDescriptorBufferInfo descriptorBuffersInfos[16];
	uint32_t descriptorBuffersInfosCount = 0;
	VkDescriptorImageInfo descriptorImagesInfos[16];
	uint32_t descriptorImagesInfosCount = 0;

public:
	void AddImagesBinding( const GfxImageSamplerCombined* images, uint32_t count, uint32_t binding, eDescriptorType type, eDescriptorAccess access );
	void AddBuffersBinding( const GpuBuffer* buffers, uint32_t count, uint32_t binding, eDescriptorType type, eDescriptorAccess access );
	//TODO: I could maybe use the union GpuInputDataEntry instead of void* ...
	void AddBinding( const void* data, uint32_t count, uint32_t binding, eDescriptorType type, eDescriptorAccess access );
	void Submit( GfxDescriptorTable descriptorTable );
};

void CreateDescriptorPool( uint32_t uniformBuffersCount, uint32_t uniformBufferDynamicCount, uint32_t combinedImageSamplerCount, uint32_t storageImageCount, uint32_t sampledImageCount, uint32_t maxSets, VkDescriptorPool * o_descriptorPool );
void CreateDesciptorTableLayout( const VkDescriptorSetLayoutBinding* bindings, uint32_t count, GfxDescriptorTableLayout* o_layout );
void CreateDescriptorTables( GfxDescriptorPool descriptorPool, uint32_t count, GfxDescriptorTableLayout * descriptorSetLayouts, GfxDescriptorTable* o_descriptorTables );
void UpdateDescriptorTables( size_t writeDescriptorTableCount, const WriteDescriptorTable* writeDescriptorTable, GfxDescriptorTable* descriptorTable );

void Destroy( GfxDescriptorPool* descriptorPool );

constexpr uint32_t VI_STATE_MAX_DESCRIPTIONS = 5;
struct VIState
{
	VkVertexInputBindingDescription vibDescription[VI_STATE_MAX_DESCRIPTIONS];
	uint32_t vibDescriptionsCount;
	VkVertexInputAttributeDescription visDescriptions[VI_STATE_MAX_DESCRIPTIONS];
	uint32_t visDescriptionsCount;
};

struct ShaderCreation
{
	std::vector<char> code;
	const char* entryPoint;
	GfxShaderStageFlagBits flags;
};

struct RasterizationState
{
	bool depthBiased;
	bool backFaceCulling;
};

struct DepthStencilState
{
	bool depthRead;
	bool depthWrite;
	GfxCompareOp depthCompareOp;
};

struct GpuPipelineLayout
{
	std::vector<GfxRootConstantRange> RootConstantRanges;
};

enum class GfxPrimitiveTopology {
	POINT_LIST = VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
	LINE_LIST = VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
	LINE_STRIP = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,
	TRIANGLE_LIST = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
	TRIANGLE_STRIP = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
	TRIANGLE_FAN = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN,
};

inline VkPrimitiveTopology ToVkPrimitiveTopology( GfxPrimitiveTopology primitiveTopology )
{
	return static_cast< VkPrimitiveTopology >(primitiveTopology);
}

struct GpuPipelineStateDesc
{
	VIState viState;
	std::vector<ShaderCreation> shaders;
	RasterizationState rasterizationState;
	DepthStencilState depthStencilState;
	bool blendEnabled;
	GfxPrimitiveTopology primitiveTopology;
};

struct FrameBuffer
{
	VkFramebuffer frameBuffer;
	VkExtent2D extent;
	uint32_t layerCount;
	uint32_t colorCount;
	uint32_t depthCount;
};

struct RenderPass {
	VkRenderPass vk_renderpass;
	std::vector<VkFormat> colorFormats;
	VkFormat depthFormat;
	FrameBuffer outputFrameBuffer[SIMULTANEOUS_FRAMES];
};

typedef VkPipeline GfxPipeline;

void Destroy( GfxPipeline* pipeline );
void Destroy( GfxPipelineLayout* pipelineLayout );
void Destroy( GfxDescriptorTableLayout* layout );
void Destroy( GfxDescriptorTable* descriptorTables, uint32_t count, GfxDescriptorPool descriptorPool );

void CmdBindPipeline( GfxCommandBuffer commandBuffer, GfxPipelineBindPoint pipelineBindPoint, GfxPipeline pipeline );
void CmdBindDescriptorTable( GfxCommandBuffer commandBuffer, GfxPipelineBindPoint pipelineBindPoint, GfxPipelineLayout pipelineLayout, uint32_t rootBindingPoint, GfxDescriptorTable descriptorTable );

typedef VkSemaphore GfxSemaphore;
bool CreateGfxSemaphore( GfxSemaphore* pSemaphore );
void DestroyGfxSemaphore( GfxSemaphore* pSemaphore );

typedef VkFence GfxFence;
bool CreateGfxFence( GfxFence* pFence );
void DestroyGfxFence( GfxFence* pFence );
void ResetGfxFences( const GfxFence* pFences, uint32_t fencesCount );
void WaitForFence( const GfxFence* pFences, uint32_t fenceCount, uint64_t timeoutNS );
void WaitForFence( const GfxFence* pFences, uint32_t fenceCount );

bool QueueSubmit( VkQueue queue, GfxCommandBuffer* commandBuffers, uint32_t commandBuffersCount, GfxSemaphore* pWaitSemaphores, GfxPipelineStageFlag* waitDstStageMask, uint32_t waitSemaphoresCount, GfxSemaphore* pSignalSemaphores, uint32_t signalSemaphoresCount, GfxFence signalFence );

typedef VkSwapchainKHR GfxSwapchain;
typedef VkResult GfxSwapchainOperationResult;
struct GfxSwapchainImage
{
	VkSwapchainKHR swapchain;
	uint32_t imageIndex;
};

bool SwapchainImageIsValid( GfxSwapchainOperationResult result );
GfxSwapchainOperationResult AcquireNextSwapchainImage( GfxSwapchain swapchain, GfxSemaphore signalSemaphore, GfxSwapchainImage* swapchainImage );
GfxSwapchainOperationResult QueuePresent( VkQueue presentQueue, const GfxSwapchainImage& swapchainImage, GfxSemaphore* pWaitSemaphores, uint32_t waitSemaphoresCount );

void CmdBindVertexInputs( GfxCommandBuffer commandBuffer, GfxApiBuffer* pVertexBuffers, uint32_t firstBinding, uint32_t vertexBuffersCount, GfxDeviceSize* pBufferOffsets );
void CmdBindIndexBuffer( GfxCommandBuffer commandBuffer, GfxApiBuffer buffer, GfxDeviceSize bufferOffset, GfxIndexType indexType );
void CmdDrawIndexed( GfxCommandBuffer commandBuffer, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance );

void DeviceWaitIdle( GfxDevice device );

bool CreateCommandBuffers( GfxCommandPool commandPool, GfxCommandBuffer* pCommandBuffers, uint32_t count );
void DestroyCommandBuffers( GfxCommandPool commandPool, GfxCommandBuffer* pCommandBuffers, uint32_t count );

PhysicalDevice PickSuitablePhysicalDevice( DisplaySurface swapchainSurface, GpuInstance& instance );
Device create_logical_device( DisplaySurface swapchainSurface, PhysicalDevice physicalDevice, bool enableValidationLayers );
void Destroy( Device* device );

GpuInstance CreateInstance( bool useValidationLayer );
void Destroy( GpuInstance* gpuInstance );

struct Gfx_Globals {
	GpuInstance instance = {};
	PhysicalDevice physicalDevice = VK_NULL_HANDLE;
	Device device = {};
	GfxCommandPool graphicsSingleUseCommandPool = VK_NULL_HANDLE;
};

extern Gfx_Globals g_gfx;