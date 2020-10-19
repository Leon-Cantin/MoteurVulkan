#include "classic_2d_renderer_imp.h"

#include "frame_graph_script.h"
#include "renderer.h"
#include "descriptors.h"
#include "profile.h"
#include "console_command.h"
#include "window_handler.h"
#include "allocators.h"
#include "memory.h"

#include <glm/glm.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/matrix_transform.hpp>

VkDescriptorPool descriptorPool_scene;

extern Swapchain g_swapchain;

std::array< GpuInputData, SIMULTANEOUS_FRAMES> _inputBuffers;

/*
	Update Stuff
*/

static void UpdateSceneUniformBuffer(const glm::mat4& world_view_matrix, VkExtent2D extent, GpuBuffer* sceneUniformBuffer)
{
	SceneMatricesUniform sceneMatrices = {};
	//sceneMatrices.view = world_view_matrix;
	//Layout is columns are horizontal in memory
	//float ratio = (float)extent.width / extent.height;
	//float height = 50.0f;
	//float width = height * ratio;
	float factor = 1.0;
	float width = screenSize.width * factor;
	float height = screenSize.height * factor;

	float halfHeight = height / 2.0f;
	float halfWidth = width / 2.0f;

	sceneMatrices.proj = glm::ortho( -halfWidth, halfWidth, -halfHeight, halfHeight, 0.0f, 10.0f );
	sceneMatrices.proj[1][1] *= -1;//Compensate for OpenGL Y coordinate being inverted
	UpdateGpuBuffer( sceneUniformBuffer, &sceneMatrices, sizeof( sceneMatrices ), 0 );
}

//Updates the data of a single instance (a draw or an object)
static void UpdateGfxInstanceData( const GfxAssetInstance* assetInstance, SceneInstanceSet* sceneInstanceDescriptorSet, BufferAllocator* allocator )
{
	GfxInstanceData instanceData = {};
	instanceData.model = ComputeSceneInstanceModelMatrix( assetInstance->instanceData );
	for( uint32_t i = 0; i < assetInstance->asset->textureIndices.size(); ++i )
		instanceData.texturesIndexes[i] = assetInstance->asset->textureIndices[i];
	instanceData.dithering[0] = assetInstance->useDithering ? 1 : 0;

	size_t allocationSize = sizeof( GfxInstanceData );
	size_t memoryOffset = AllocateGpuBufferSlot( allocator, allocationSize );

	sceneInstanceDescriptorSet->geometryBufferOffsets = memoryOffset;
	UpdateGpuBuffer( allocator->buffer, &instanceData, allocationSize, memoryOffset );
}

//TODO seperate the buffer update and computation of frame data
static void updateUniformBuffer( uint32_t currentFrame, const SceneInstance* cameraSceneInstance, const std::vector<GfxAssetInstance>& drawList, std::vector<DrawListEntry>& o_drawlist, const std::vector<TextZone>& textZones )
{
	glm::mat4 world_view_matrix = ComputeCameraSceneInstanceViewMatrix( *cameraSceneInstance );

	GpuInputData currentGpuInputData = _inputBuffers[currentFrame];
	VkExtent2D viewportExtent = GetImage( &currentGpuInputData, eTechniqueDataEntryImageName::SCENE_COLOR )->image->extent;

	BufferAllocator allocator { GetBuffer( &currentGpuInputData, eTechniqueDataEntryName::INSTANCE_DATA ) };
	for( uint32_t i = 0; i < drawList.size(); ++i )
	{
		SceneInstanceSet instanceDescSet;
		UpdateGfxInstanceData( &drawList[i], &instanceDescSet, &allocator );

		o_drawlist[i] = { drawList[i].asset, instanceDescSet };
	}

	UpdateSceneUniformBuffer( world_view_matrix, viewportExtent, GetBuffer( &currentGpuInputData, eTechniqueDataEntryName::SCENE_DATA ) );

	if( textZones.size() > 0 )
		UpdateText( textZones.data(), textZones.size(), viewportExtent );
}

static void PrepareSceneFrameData( SceneFrameData* frameData, uint32_t currentFrame, const SceneInstance* cameraSceneInstance, const std::vector<GfxAssetInstance>& drawList, const std::vector<TextZone>& textZones )
{
	frameData->drawList.resize( drawList.size() );
	updateUniformBuffer( currentFrame, cameraSceneInstance, drawList, frameData->drawList, textZones );
}

GfxImageSamplerCombined textTextures[1];

static void FillInputBuffers( BindlessTexturesState* bindlessTexturesState )
{

	VkSampler sampler = GetSampler( eSamplers::Trilinear );

	textTextures[0] = { const_cast< GfxImage*>(GetTextImage()), sampler };
	
	for( size_t i = 0; i < SIMULTANEOUS_FRAMES; ++i )
	{
		SetImages( &_inputBuffers[i], eTechniqueDataEntryImageName::BINDLESS_TEXTURES, bindlessTexturesState->_bindlessTextures, bindlessTexturesState->_bindlessTexturesCount );
		SetImages( &_inputBuffers[i], eTechniqueDataEntryImageName::TEXT, textTextures, 1 );
	}
}

static bool NeedResize()
{
	bool value = WH::framebuffer_resized;
	WH::framebuffer_resized = false;
	return value;
}

static VkDescriptorPool CreateDescriptorPool_BAD()
{
	const uint32_t geometryDescriptorSets = 2 * SIMULTANEOUS_FRAMES;
	const uint32_t geometryBuffersCount = 2 * SIMULTANEOUS_FRAMES;
	const uint32_t geometryImageCount = 64 * SIMULTANEOUS_FRAMES;
	const uint32_t geomtryDynamicBuffersCount = 1 * SIMULTANEOUS_FRAMES;

	const uint32_t textDescriptorSetsCount = 2;
	const uint32_t textImageCount = 2;

	const uint32_t copyDescriptorSetsCount = 8;

	const uint32_t uniformBuffersCount = geometryBuffersCount;
	const uint32_t imageSamplersCount = geometryImageCount + textImageCount;
	const uint32_t storageImageCount = 1;
	const uint32_t uniformBuffersDynamicCount = geomtryDynamicBuffersCount;
	const uint32_t sampledImageCount = 8;

	const uint32_t maxSets = geometryDescriptorSets + textDescriptorSetsCount + copyDescriptorSetsCount;

	VkDescriptorPool descriptorPool;
	CreateDescriptorPool( uniformBuffersCount, uniformBuffersDynamicCount, imageSamplersCount, storageImageCount, sampledImageCount, maxSets, &descriptorPool );

	return descriptorPool;
}

void InitRendererImp( DisplaySurface swapchainSurface )
{
	InitRenderer( swapchainSurface, NeedResize, WH::GetFramebufferSize );

	LoadFontTexture();
	CreateTextVertexBuffer( 256 );
}

void CompileScene( BindlessTexturesState* bindlessTexturesState )
{
	CleanupFrameGraph();

	if( descriptorPool_scene )
		vkDestroyDescriptorPool( g_gfx.device.device, descriptorPool_scene, nullptr );
	descriptorPool_scene = CreateDescriptorPool_BAD();

	MEM::zero( &_inputBuffers );

	FillInputBuffers( bindlessTexturesState );

	FG_Script_SetInputBuffers( &_inputBuffers, descriptorPool_scene );
	CompileFrameGraph( InitializeScript );
}

void CleanupRendererImp()
{
	CleanupTextRenderPass();
	CleanupRenderer();	
	vkDestroyDescriptorPool( g_gfx.device.device, descriptorPool_scene, nullptr );
}

void DrawFrame( uint32_t currentFrame, const SceneInstance* cameraSceneInstance, const std::vector<GfxAssetInstance>& drawList, const std::vector<TextZone>& textZones )
{
	WaitForFrame(currentFrame);

	SceneFrameData frameData;
	PrepareSceneFrameData(&frameData, currentFrame, cameraSceneInstance, drawList, textZones );

	draw_frame(currentFrame, &frameData);
}