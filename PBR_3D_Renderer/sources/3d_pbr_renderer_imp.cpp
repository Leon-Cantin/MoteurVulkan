#include "3d_pbr_renderer_imp.h"

#include "frame_graph_script.h"
#include "renderer.h"
#include "profile.h"
#include "console_command.h"
#include "window_handler.h"
#include "allocators.h"

#include <glm/glm.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/matrix_transform.hpp>

GfxDescriptorPool descriptorPool;

extern Swapchain g_swapchain;
const DisplaySurface* m_swapchainSurface;


std::array< GpuInputData, SIMULTANEOUS_FRAMES> _inputBuffers;

/*
	Update Stuff
*/
static void UpdateLightUniformBuffer( const SceneMatricesUniform* shadowSceneMatrices, LightUniform* light, GpuBuffer* lightUniformBuffer, GpuBuffer* shadowSceneUniformBuffer )
{
	const glm::mat4 biasMat = {
		0.5, 0.0, 0.0, 0.0,
		0.0, 0.5, 0.0, 0.0,
		0.0, 0.0, 1.0, 0.0,
		0.5, 0.5, 0.0, 1.0 };
	light->shadowMatrix = biasMat * shadowSceneMatrices->proj * shadowSceneMatrices->view;
	UpdateGpuBuffer( lightUniformBuffer, light, sizeof( LightUniform ), 0 );
	UpdateShadowUniformBuffers( shadowSceneUniformBuffer, shadowSceneMatrices);
}

static void UpdateSceneUniformBuffer(const glm::mat4& world_view_matrix, VkExtent2D extent, GpuBuffer* sceneUniformBuffer)
{
	SceneMatricesUniform sceneMatrices = {};
	sceneMatrices.view = world_view_matrix;
	sceneMatrices.proj = glm::perspective(glm::radians(45.0f), extent.width / (float)extent.height, 0.1f, 10.0f);
	sceneMatrices.proj[1][1] *= -1;//Compensate for OpenGL Y coordinate being inverted
	UpdateGpuBuffer( sceneUniformBuffer, &sceneMatrices, sizeof( sceneMatrices ), 0 );
}

static void UpdateGfxInstanceData( const GfxAssetInstance& assetInstance, SceneInstanceSet* sceneInstanceDescriptorSet, BufferAllocator* allocator )
{
	GfxInstanceData instanceMatrices = {};
	instanceMatrices.model = ComputeSceneInstanceModelMatrix( assetInstance.instanceData );
	for( uint32_t i = 0; i < assetInstance.asset->textureIndices.size(); ++i )
		instanceMatrices.texturesIndexes[i] = assetInstance.asset->textureIndices[i];

	size_t allocationSize = sizeof( GfxInstanceData );
	size_t memoryOffset = AllocateGpuBufferSlot( allocator, allocationSize );

	sceneInstanceDescriptorSet->geometryBufferOffsets = memoryOffset;
	UpdateGpuBuffer( allocator->buffer, &instanceMatrices, allocationSize, memoryOffset );
}

static void updateTextOverlayBuffer( uint32_t currentFrame )
{
	float miliseconds = GetTimestampsDelta( Timestamp::COMMAND_BUFFER_START, Timestamp::COMMAND_BUFFER_END, abs( static_cast< int64_t >(currentFrame) - 1ll ) );
	char textBuffer[256];
	int charCount = sprintf_s( textBuffer, 256, "GPU: %4.4fms", miliseconds );
	size_t textZonesCount = 1;
	TextZone textZones[2] = { -1.0f, -1.0f, std::string( textBuffer ) };
	if( ConCom::isOpen() ) {
		textZones[1] = { -1.0f, 0.0f, ConCom::GetViewableString() };
		++textZonesCount;
	}
	UpdateText( textZones, textZonesCount, g_swapchain.extent );
}

//TODO seperate the buffer update and computation of frame data
//TODO Make light Uniform const
static void updateUniformBuffer( uint32_t currentFrame, const SceneInstance* cameraSceneInstance, LightUniform* light, const std::vector<GfxAssetInstance>& drawList, std::vector<DrawListEntry>& o_drawlist )
{
	glm::mat4 world_view_matrix = ComputeCameraSceneInstanceViewMatrix( *cameraSceneInstance );

	VkExtent2D swapChainExtent = g_swapchain.extent;

	GpuInputData currentGpuInputData = _inputBuffers[currentFrame];

	BufferAllocator allocator { GetBuffer( &currentGpuInputData, eTechniqueDataEntryName::INSTANCE_DATA ) };
	for( uint32_t i = 0; i < drawList.size(); ++i )
	{
		SceneInstanceSet instanceDescSet;
		UpdateGfxInstanceData( drawList[i], &instanceDescSet, &allocator );

		o_drawlist[i] = { drawList[i].asset, instanceDescSet };
	}

	UpdateSceneUniformBuffer( world_view_matrix, swapChainExtent, GetBuffer( &currentGpuInputData, eTechniqueDataEntryName::SCENE_DATA ) );

	SceneMatricesUniform shadowSceneMatrices;
	computeShadowMatrix( light->position, &shadowSceneMatrices.view, &shadowSceneMatrices.proj );

	UpdateLightUniformBuffer( &shadowSceneMatrices, light, GetBuffer( &currentGpuInputData, eTechniqueDataEntryName::LIGHT_DATA ), GetBuffer( &currentGpuInputData, eTechniqueDataEntryName::SHADOW_DATA ) );

	UpdateSkyboxUniformBuffers( GetBuffer( &currentGpuInputData, eTechniqueDataEntryName::SKYBOX_DATA ), world_view_matrix );

	updateTextOverlayBuffer( currentFrame );
}

static void PrepareSceneFrameData( SceneFrameData* frameData, uint32_t currentFrame, const SceneInstance* cameraSceneInstance, LightUniform* light, const std::vector<GfxAssetInstance>& drawList )
{
	frameData->drawList.resize( drawList.size() );
	updateUniformBuffer( currentFrame, cameraSceneInstance, light, drawList, frameData->drawList );
}

GfxImageSamplerCombined textTextures[1];
GfxImageSamplerCombined skyboxImages[1];

static void CreateBuffers( BindlessTexturesState* bindlessTexturesState, const GfxImage* skyboxImage )
{
	CreateTextVertexBuffer( 256 );

	VkSampler sampler = GetSampler( eSamplers::Trilinear );

	textTextures[0] = { const_cast< GfxImage*>(GetTextImage()), sampler };
	skyboxImages[0] = { const_cast< GfxImage* >(skyboxImage), sampler };
	
	for( size_t i = 0; i < SIMULTANEOUS_FRAMES; ++i )
	{
		SetImages( &_inputBuffers[i], eTechniqueDataEntryImageName::BINDLESS_TEXTURES, bindlessTexturesState->_bindlessTextures, bindlessTexturesState->_bindlessTexturesCount );
		SetImages( &_inputBuffers[i], eTechniqueDataEntryImageName::TEXT, textTextures, 1 );
		SetImages( &_inputBuffers[i], eTechniqueDataEntryImageName::SKYBOX, skyboxImages, 1 );
		SetSamplers( &_inputBuffers[i], eTechniqueDataEntryName::SAMPLERS, GetSamplers(), 2 );
	}
}

static bool NeedResize()
{
	bool value = WH::framebuffer_resized;
	WH::framebuffer_resized = false;
	return value;
}

void InitRendererImp( const DisplaySurface* swapchainSurface )
{
	m_swapchainSurface = swapchainSurface;
	InitRenderer( *m_swapchainSurface, NeedResize, WH::GetFramebufferSize );

	LoadFontTexture();
	CreateTextVertexBuffer( 256 );
}

static GfxDescriptorPool CreateDescriptorPool_BAD()
{
	const uint32_t geometryDescriptorSets = 2 * SIMULTANEOUS_FRAMES;
	const uint32_t geometryBuffersCount = 2 * SIMULTANEOUS_FRAMES;
	const uint32_t geometryImageCount = 20 * SIMULTANEOUS_FRAMES;
	const uint32_t geomtryDynamicBuffersCount = 1 * SIMULTANEOUS_FRAMES;

	const uint32_t shadowDescriptorSets = 2 * SIMULTANEOUS_FRAMES;
	const uint32_t shadowBuffersCount = 2 * SIMULTANEOUS_FRAMES;
	const uint32_t shadowDynamicBuffersCount = 1 * SIMULTANEOUS_FRAMES;

	const uint32_t skyboxDescriptorSetsCount = 2 * SIMULTANEOUS_FRAMES;
	const uint32_t skyboxBuffersCount = 2 * SIMULTANEOUS_FRAMES;
	const uint32_t skyboxImageCount = 2 * SIMULTANEOUS_FRAMES;

	const uint32_t textDescriptorSetsCount = 2;
	const uint32_t textImageCount = 2;

	const uint32_t uniformBuffersCount = geometryBuffersCount + shadowBuffersCount + skyboxBuffersCount;
	const uint32_t imageSamplersCount = geometryImageCount + skyboxImageCount + textImageCount;
	const uint32_t storageImageCount = 1;
	const uint32_t uniformBuffersDynamicCount = geomtryDynamicBuffersCount + shadowDynamicBuffersCount;
	const uint32_t sampledImageCount = 6;

	const uint32_t maxSets = geometryDescriptorSets + shadowDescriptorSets + skyboxDescriptorSetsCount + textDescriptorSetsCount;

	GfxDescriptorPool descriptorPool;
	CreateDescriptorPool( uniformBuffersCount, uniformBuffersDynamicCount, imageSamplersCount, storageImageCount, sampledImageCount, maxSets, &descriptorPool );

	return descriptorPool;
}

void CompileScene( BindlessTexturesState* bindlessTexturesState, const GfxImage* skyboxImage )
{
	if( descriptorPool )
		Destroy( &descriptorPool );
	descriptorPool = CreateDescriptorPool_BAD();

	CreateBuffers( bindlessTexturesState, skyboxImage );
	FG_Script_SetInputBuffers( &_inputBuffers, descriptorPool );
	CompileFrameGraph( InitializeScript );
}

void CleanupRendererImp()
{
	CleanupTextRenderPass();
	CleanupRenderer();	
	Destroy( &descriptorPool );
}

void DrawFrame( uint32_t currentFrame, const SceneInstance* cameraSceneInstance, LightUniform* light, const std::vector<GfxAssetInstance>& drawList )
{
	WaitForFrame(currentFrame);

	SceneFrameData frameData;
	PrepareSceneFrameData(&frameData, currentFrame, cameraSceneInstance, light, drawList);

	draw_frame(currentFrame, &frameData);
}