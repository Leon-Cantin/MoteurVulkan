#include "scene2D_renderer_imp.h"

#include "vk_buffer.h"
#include "frame_graph_script.h"
#include "renderer.h"
#include "descriptors.h"
#include "profile.h"
#include "console_command.h"

#include <glm/glm.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/matrix_transform.hpp>

VkDescriptorPool descriptorPool;

extern Swapchain g_swapchain;

bool g_forceReloadShaders = false;

SceneInstanceSet g_sceneInstanceDescriptorSets[5];
size_t g_sceneInstancesCount = 0;
std::array< InputBuffers, SIMULTANEOUS_FRAMES> _inputBuffers;

/*
	Create Stuff
*/

SceneInstanceSet*  CreateGeometryInstanceDescriptorSet()
{
	SceneInstanceSet* sceneInstanceDescriptorSet = &g_sceneInstanceDescriptorSets[g_sceneInstancesCount++];
	CreateSceneInstanceDescriptorSet(sceneInstanceDescriptorSet);
	return sceneInstanceDescriptorSet;
}

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

static void UpdateGeometryUniformBuffer( const SceneInstance* sceneInstance, const SceneInstanceSet* sceneInstanceDescriptorSet, GpuBuffer* geometryUniformBuffer )
{
	InstanceMatrices instanceMatrices = {};
	instanceMatrices.model = ComputeSceneInstanceModelMatrix( *sceneInstance );

	UpdateGpuBuffer( geometryUniformBuffer, &instanceMatrices, sizeof( InstanceMatrices ), sceneInstanceDescriptorSet->geometryBufferOffsets[0] );//TODO checking only 0 is stupid, why do I have one for each frame?
}

void ReloadSceneShaders()
{
	vkDeviceWaitIdle(g_vk.device);
	VkExtent2D extent = g_swapchain.extent;
	ReloadSkyboxShaders(extent);
	ReloadGeometryShaders(extent);
}

VkDescriptorImageInfo albedos[5];
VkDescriptorImageInfo normalTextures[1];
VkDescriptorImageInfo textTextures[1];
VkDescriptorImageInfo skyboxImages[1];

static void CreateBuffers( const GfxImage* albedoImage, const GfxImage* normalImage, const GfxImage* skyboxImage )
{
	CreateTextVertexBuffer( 256 );

	VkSampler sampler = GetSampler( Samplers::Trilinear );

	albedos[0] = { sampler, albedoImage->imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
	albedos[1] = { sampler, albedoImage->imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
	albedos[2] = { sampler, albedoImage->imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
	albedos[3] = { sampler, albedoImage->imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
	albedos[4] = { sampler, albedoImage->imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
	normalTextures[0] = { sampler, normalImage->imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
	textTextures[0] = { sampler, GetTextImage()->imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
	skyboxImages[0] = { sampler, skyboxImage->imageView };
	
	for( size_t i = 0; i < SIMULTANEOUS_FRAMES; ++i )
	{
		SetImages( &_inputBuffers[i], eTechniqueDataEntryImageName::ALBEDOS, albedos);
		SetImages( &_inputBuffers[i], eTechniqueDataEntryImageName::NORMALS, normalTextures);
		SetImages( &_inputBuffers[i], eTechniqueDataEntryImageName::TEXT, textTextures);
		SetImages( &_inputBuffers[i], eTechniqueDataEntryImageName::SKYBOX, skyboxImages );
	}
}

void InitRendererImp()
{
	InitRenderer();

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

	const uint32_t maxSets = geometryDescriptorSets + shadowDescriptorSets + skyboxDescriptorSetsCount + textDescriptorSetsCount;

	createDescriptorPool(uniformBuffersCount, uniformBuffersDynamicCount, imageSamplersCount, storageImageCount, maxSets, &descriptorPool);

	LoadFontTexture();
}

void CompileScene( const GfxImage* albedoImage, const GfxImage* normalImage, const GfxImage* skyboxImage )
{
	CreateBuffers( albedoImage, normalImage, skyboxImage );
	SetInputBuffers( &_inputBuffers, descriptorPool );
	CompileFrameGraph( InitializeScript );
}

void CleanupRendererImp()
{
	CleanupRenderer();	
	vkDestroyDescriptorPool(g_vk.device, descriptorPool, nullptr);
}

static void updateTextOverlayBuffer(uint32_t currentFrame)
{
	float miliseconds = GetTimestampsDelta(Timestamp::COMMAND_BUFFER_START, Timestamp::COMMAND_BUFFER_END, abs(static_cast<int64_t>(currentFrame) - 1ll));
	char textBuffer[256];
	int charCount = sprintf_s(textBuffer, 256, "GPU: %4.4fms", miliseconds);
	size_t textZonesCount = 1;
	TextZone textZones[2] = { -1.0f, -1.0f, std::string(textBuffer) };
	if (ConCom::isOpen()) {
		textZones[1] = { -1.0f, 0.0f, ConCom::GetViewableString() };
		++textZonesCount;
	}
	UpdateText(textZones, textZonesCount, g_swapchain.extent);
}

//TODO seperate the buffer update and computation of frame data
//TODO Make light Uniform const
//TODO Instead of preassigning descriptors to instances, just give them one before drawing
static void updateUniformBuffer( uint32_t currentFrame, const SceneInstance* cameraSceneInstance, LightUniform* light, const std::vector<std::pair<const SceneInstance*,const SceneRenderableAsset*>>& drawList )
{
	glm::mat4 world_view_matrix = ComputeCameraSceneInstanceViewMatrix(*cameraSceneInstance);

	VkExtent2D swapChainExtent = g_swapchain.extent;

	InputBuffers inputbuffers = _inputBuffers[currentFrame];

	//Update GeometryUniformBuffer
	for( auto& drawNode : drawList )
		UpdateGeometryUniformBuffer( drawNode.first, drawNode.second->descriptorSet, GetBuffer( &inputbuffers, eTechniqueDataEntryName::INSTANCE_DATA) );

	UpdateSceneUniformBuffer(world_view_matrix, swapChainExtent, GetBuffer( &inputbuffers, eTechniqueDataEntryName::SCENE_DATA ) );

	SceneMatricesUniform shadowSceneMatrices;
	computeShadowMatrix(light->position, &shadowSceneMatrices.view, &shadowSceneMatrices.proj);

	UpdateLightUniformBuffer(&shadowSceneMatrices, light, GetBuffer( &inputbuffers, eTechniqueDataEntryName::LIGHT_DATA ), GetBuffer( &inputbuffers, eTechniqueDataEntryName::SHADOW_DATA ) );

	UpdateSkyboxUniformBuffers( GetBuffer( &inputbuffers, eTechniqueDataEntryName::SKYBOX_DATA ), world_view_matrix );

	updateTextOverlayBuffer(currentFrame);
}

static void PrepareSceneFrameData(SceneFrameData* frameData, uint32_t currentFrame, const SceneInstance* cameraSceneInstance, LightUniform* light, const std::vector<std::pair<const SceneInstance*, const SceneRenderableAsset*>>& drawList)
{
	updateUniformBuffer(currentFrame, cameraSceneInstance, light,drawList);

	frameData->renderableAssets.resize(drawList.size());
	for (size_t i = 0; i < drawList.size(); ++i)
		frameData->renderableAssets[i] = drawList[i].second;
}

void DrawFrame( uint32_t currentFrame, const SceneInstance* cameraSceneInstance, LightUniform* light, const std::vector<std::pair<const SceneInstance*, const SceneRenderableAsset*>>& drawList )
{
	if (g_forceReloadShaders)
	{
		g_forceReloadShaders = false;
		ReloadSceneShaders();
	}
	WaitForFrame(currentFrame);

	SceneFrameData frameData;
	PrepareSceneFrameData(&frameData, currentFrame, cameraSceneInstance, light, drawList);

	draw_frame(currentFrame, &frameData);
}

void ForceReloadShaders()
{
	g_forceReloadShaders = true;
}