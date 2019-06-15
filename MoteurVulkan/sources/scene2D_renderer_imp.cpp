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

PerFrameBuffer sceneUniformBuffer;
PerFrameBuffer lightUniformBuffer;
PerFrameBuffer instanceMatricesBuffer;

VkDescriptorPool descriptorPool;

extern Swapchain g_swapchain;

bool g_forceReloadShaders = false;

/*
	Create Stuff
*/
static void CreateInstanceMatricesBuffers(uint32_t maxModelsCount)
{
	CreatePerFrameBuffer(sizeof(InstanceMatrices)*maxModelsCount, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &instanceMatricesBuffer);
}

static void CreateGeometryUniformBuffer()
{
	CreatePerFrameBuffer(sizeof(SceneMatricesUniform), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &sceneUniformBuffer);
	CreatePerFrameBuffer(sizeof(LightUniform), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &lightUniformBuffer);
}

void CreateGeometryRenderpassDescriptorSet(const GfxImage* albedoImage, const GfxImage* normalImage)
{
	const GfxImage *shadowImages = FG::GetRenderTarget(RT_SHADOW_MAP);

	CreateGeometryDescriptorSet(descriptorPool, sceneUniformBuffer.buffers.data(), instanceMatricesBuffer.buffers.data(), lightUniformBuffer.buffers.data(), albedoImage->imageView, normalImage->imageView, GetSampler(Samplers::Trilinear),
		shadowImages->imageView, GetSampler(Samplers::Shadow));

	CreateShadowDescriptorSet(descriptorPool, instanceMatricesBuffer.buffers.data());
}

void CreateGeometryInstanceDescriptorSet(SceneInstanceSet* sceneInstanceDescriptorSet)
{
	CreateSceneInstanceDescriptorSet(sceneInstanceDescriptorSet);
}

void UpdateGeometryUniformBuffer(const SceneInstance* sceneInstance, const SceneInstanceSet* sceneInstanceDescriptorSet, uint32_t currentFrame)
{
	InstanceMatrices instanceMatrices = {};
	instanceMatrices.model = ComputeSceneInstanceModelMatrix(*sceneInstance);

	VkDeviceSize frameMemoryOffset = GetMemoryOffsetForFrame(&instanceMatricesBuffer, currentFrame);
	void* data;
	vkMapMemory(g_vk.device, instanceMatricesBuffer.memory, sceneInstanceDescriptorSet->geometryBufferOffsets[currentFrame] + frameMemoryOffset, sizeof(InstanceMatrices), 0, &data);
	memcpy(data, &instanceMatrices, sizeof(InstanceMatrices));
	vkUnmapMemory(g_vk.device, instanceMatricesBuffer.memory);
}

void InitSkybox(const GfxImage* skyboxImage)
{
	CreateSkyboxDescriptorSet(descriptorPool, skyboxImage->imageView, GetSampler(Samplers::Trilinear));
}

/*
	Update Stuff
*/
void UpdateLightUniformBuffer(const SceneMatricesUniform* shadowSceneMatrices, LightUniform* light, uint32_t currentFrame)
{
	const glm::mat4 biasMat = {
		0.5, 0.0, 0.0, 0.0,
		0.0, 0.5, 0.0, 0.0,
		0.0, 0.0, 1.0, 0.0,
		0.5, 0.5, 0.0, 1.0 };
	light->shadowMatrix = biasMat * shadowSceneMatrices->proj * shadowSceneMatrices->view;
	UpdatePerFrameBuffer(&lightUniformBuffer, light, sizeof(LightUniform), currentFrame);

	UpdateShadowUniformBuffers(currentFrame, shadowSceneMatrices);
}

void UpdateSceneUniformBuffer(const glm::mat4& world_view_matrix, VkExtent2D extent, uint32_t currentFrame)
{
	SceneMatricesUniform sceneMatrices = {};
	sceneMatrices.view = world_view_matrix;
	sceneMatrices.proj = glm::perspective(glm::radians(45.0f), extent.width / (float)extent.height, 0.1f, 10.0f);
	sceneMatrices.proj[1][1] *= -1;//Compensate for OpenGL Y coordinate being inverted
	UpdatePerFrameBuffer(&sceneUniformBuffer, &sceneMatrices, sizeof(sceneMatrices), currentFrame);
}

void ReloadSceneShaders()
{
	vkDeviceWaitIdle(g_vk.device);
	VkExtent2D extent = g_swapchain.extent;
	ReloadSkyboxShaders(extent);
	ReloadGeometryShaders(extent);
}

void InitRendererImp()
{
	InitRenderer(InitializeScript);

	const uint32_t geometryDescriptorSets = 2 * SIMULTANEOUS_FRAMES;
	const uint32_t geometryBuffersCount = 2 * SIMULTANEOUS_FRAMES;
	const uint32_t geometryImageCount = 20 * SIMULTANEOUS_FRAMES;
	const uint32_t geomtryDynamicBuffersCount = 1 * SIMULTANEOUS_FRAMES;

	const uint32_t shadowDescriptorSets = 2 * SIMULTANEOUS_FRAMES;
	const uint32_t shadowBuffersCount = 2 * SIMULTANEOUS_FRAMES;
	const uint32_t shadowDynamicBuffersCount = 1 * SIMULTANEOUS_FRAMES;

	const uint32_t skyboxDescriptorSetsCount = 1 * SIMULTANEOUS_FRAMES;
	const uint32_t skyboxBuffersCount = 1 * SIMULTANEOUS_FRAMES;
	const uint32_t skyboxImageCount = 1 * SIMULTANEOUS_FRAMES;

	const uint32_t textDescriptorSetsCount = 1;
	const uint32_t textImageCount = 1;

	const uint32_t uniformBuffersCount = geometryBuffersCount + shadowBuffersCount + skyboxBuffersCount;
	const uint32_t imageSamplersCount = geometryImageCount + skyboxImageCount + textImageCount;
	const uint32_t storageImageCount = 1;
	const uint32_t uniformBuffersDynamicCount = geomtryDynamicBuffersCount + shadowDynamicBuffersCount;

	const uint32_t maxSets = geometryDescriptorSets + shadowDescriptorSets + skyboxDescriptorSetsCount + textDescriptorSetsCount;

	createDescriptorPool(uniformBuffersCount, uniformBuffersDynamicCount, imageSamplersCount, storageImageCount, maxSets, &descriptorPool);

	CreateGeometryUniformBuffer();
	CreateInstanceMatricesBuffers(3);
	createSkyboxUniformBuffers();
	LoadFontTexture();
	CreateTextVertexBuffer(256);
	CreateTextDescriptorSet(descriptorPool, GetSampler(Samplers::Trilinear));
}

void CleanupRendererImp()
{
	CleanupRenderer();

	DestroyPerFrameBuffer(&lightUniformBuffer);
	DestroyPerFrameBuffer(&sceneUniformBuffer);
	DestroyPerFrameBuffer(&instanceMatricesBuffer);
	
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
static void updateUniformBuffer( uint32_t currentFrame, const SceneInstance* cameraSceneInstance, LightUniform* light, const std::vector<std::pair<const SceneInstance*,const SceneRenderableAsset*>>& drawList )
{
	//camera->compute_matrix();
	glm::mat4 world_view_matrix = ComputeCameraSceneInstanceViewMatrix(*cameraSceneInstance);
	//glm::mat4 world_view_matrixx = camera.get_world_view_matrix();

	VkExtent2D swapChainExtent = g_swapchain.extent;

	//Update GeometryUniformBuffer
	for( auto& drawNode : drawList )
		UpdateGeometryUniformBuffer(drawNode.first, drawNode.second->descriptorSet, currentFrame);

	UpdateSceneUniformBuffer(world_view_matrix, swapChainExtent, currentFrame);

	SceneMatricesUniform shadowSceneMatrices;
	computeShadowMatrix(light->position, &shadowSceneMatrices.view, &shadowSceneMatrices.proj);

	UpdateLightUniformBuffer(&shadowSceneMatrices, light, currentFrame);

	UpdateSkyboxUniformBuffers(currentFrame, world_view_matrix);

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