#include "shadow_renderpass.h"

#include "descriptors.h"
#include "glm/gtc/matrix_transform.hpp"
#include "renderpass.h"
#include "vk_debug.h"
#include "vk_commands.h"
#include "framebuffer.h"
#include "vk_shader.h"
#include "file_system.h"
#include "vk_buffer.h"
#include "scene_instance.h"
#include "..\shaders\shadersCommon.h"

#include <vector>
#include <array>

VkDescriptorSetLayout shadowDescriptorSetLayout;
VkDescriptorSetLayout shadowInstanceDescriptorSetLayout;
const RenderPass* shadowRenderPass;
VkPipelineLayout shadowPipelineLayout;
VkPipeline shadowPipeline;

//TODO: this is redundant, also found in frame graph
constexpr VkFormat RT_FORMAT_SHADOW_DEPTH = VK_FORMAT_D32_SFLOAT;
constexpr VkExtent2D RT_EXTENT_SHADOW = { 1024, 1024 };

std::array<VkDescriptorSet, SIMULTANEOUS_FRAMES> shadowDescriptorSets;
std::array<VkDescriptorSet, SIMULTANEOUS_FRAMES> shadowInstanceDescriptorSet;

PerFrameBuffer shadowSceneUniformBuffer;

void computeShadowMatrix(const glm::vec3& light_location, glm::mat4* view, glm::mat4* projection)
{
	*view = glm::lookAt(light_location, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
	//TODO: encapsulate projection computation
	*projection = glm::perspective(glm::radians(65.0f), 1.0f, 0.1f, 100.0f);
	(*projection)[1][1] *= -1;//Compensate for OpenGL Y coordinate being inverted

	//return light_projection_matrix * light_view_matrix;
}

void UpdateShadowUniformBuffers(size_t currentFrame, const SceneMatricesUniform* sceneUniforms)
{
	//per Instance data should be updated by the geometry render pass
	//We only update the scene related data
	UpdatePerFrameBuffer(&shadowSceneUniformBuffer, sceneUniforms, sizeof(SceneMatricesUniform), currentFrame);
}

void CmdBeginShadowPass(VkCommandBuffer commandBuffer, size_t currentFrame)
{
	CmdBeginVkLabel(commandBuffer, "Shadow Renderpass", glm::vec4(0.5f, 0.2f, 0.4f, 1.0f));
	BeginRenderPass(commandBuffer, *shadowRenderPass, shadowRenderPass->frameBuffer.frameBuffer, shadowRenderPass->frameBuffer.extent);
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowPipeline);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowPipelineLayout, RENDERPASS_SET, 1, &shadowDescriptorSets[currentFrame], 0, nullptr);
}

void CmdDrawShadowPass(VkCommandBuffer commandBuffer, const SceneInstanceSet* instanceSet, const GfxModel* modelAsset, uint32_t currentFrame)
{
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowPipelineLayout, INSTANCE_SET, 1,
		&shadowInstanceDescriptorSet[currentFrame], 1, &instanceSet->geometryBufferOffsets[currentFrame]);
	CmdDrawIndexed(commandBuffer, *modelAsset);
}

void CmdEndShadowPass(VkCommandBuffer commandBuffer)
{
	EndRenderPass(commandBuffer);
	CmdEndVkLabel(commandBuffer);
}

void CreateShadowGraphicPipeline()
{
	auto bindingDescription = Vertex::get_binding_description();
	auto attributeDescriptions = Vertex::get_attribute_descriptions();

	std::vector<char> vertShaderCode = FS::readFile("shaders/shadows.vert.spv");
	std::vector<char> fragShaderCode;// = readFile("shaders/shadows.frag.spv"); no shadow frag

	VkDescriptorSetLayout descriptorSetLayouts[] = { shadowDescriptorSetLayout, shadowInstanceDescriptorSetLayout };
	VkPipelineLayoutCreateInfo pipeline_layout_info = {};
	pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_info.setLayoutCount = 2; // Optional
	pipeline_layout_info.pSetLayouts = descriptorSetLayouts; // Optional
	pipeline_layout_info.pushConstantRangeCount = 0; // Optional
	pipeline_layout_info.pPushConstantRanges = nullptr; // Optional

	if (vkCreatePipelineLayout(g_vk.device, &pipeline_layout_info, nullptr, &shadowPipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}

	CreatePipeline(&bindingDescription,
		attributeDescriptions.data(),
		static_cast<uint32_t>(attributeDescriptions.size()),
		vertShaderCode,
		fragShaderCode,
		RT_EXTENT_SHADOW,
		shadowRenderPass->vk_renderpass,
		shadowPipelineLayout,
		true,
		true,
		true,
		false,
		true,
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		VK_COMPARE_OP_LESS,
		&shadowPipeline);
}

void CreateShadowDescriptorSet(VkDescriptorPool descriptorPool, const VkBuffer*instanceUniformBuffer)
{
	std::array<DescriptorSet, SIMULTANEOUS_FRAMES> descriptorSets;

	for (size_t i = 0; i < SIMULTANEOUS_FRAMES; ++i)
	{
		DescriptorSet& geoDescriptorSet = descriptorSets[i] = {};
		geoDescriptorSet.descriptors.resize(1);
		geoDescriptorSet.descriptors[0] = { {shadowSceneUniformBuffer.buffers[i], 0, VK_WHOLE_SIZE}, {}, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0 };
		geoDescriptorSet.layout = shadowDescriptorSetLayout;
	}
	createDescriptorSets(descriptorPool, descriptorSets.size(), descriptorSets.data());
	for (size_t i = 0; i < SIMULTANEOUS_FRAMES; ++i)
		shadowDescriptorSets[i] = descriptorSets[i].set;

	for (size_t i = 0; i < SIMULTANEOUS_FRAMES; ++i)
	{
		DescriptorSet& geoDescriptorSet = descriptorSets[i] = {};
		geoDescriptorSet.descriptors.resize(1);
		geoDescriptorSet.descriptors[0] = { {instanceUniformBuffer[i], 0, VK_WHOLE_SIZE}, {}, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 0 };
		geoDescriptorSet.layout = shadowInstanceDescriptorSetLayout;
	}
	createDescriptorSets(descriptorPool, descriptorSets.size(), descriptorSets.data());
	for (size_t i = 0; i < SIMULTANEOUS_FRAMES; ++i)
		shadowInstanceDescriptorSet[i] = descriptorSets[i].set;
}

void CreateShadowDescriptorSetLayout()
{
	const VkDescriptorSetLayoutBinding sceneLayoutBinding = { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER , 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr };

	const std::array<VkDescriptorSetLayoutBinding, 1> bindings = { sceneLayoutBinding };
	CreateDesciptorSetLayout(bindings.data(), static_cast<uint32_t>(bindings.size()), &shadowDescriptorSetLayout);

	const VkDescriptorSetLayoutBinding instanceLayoutBinding = { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr };
	const std::array<VkDescriptorSetLayoutBinding, 1> instanceBindings = { instanceLayoutBinding };
	CreateDesciptorSetLayout(instanceBindings.data(), static_cast<uint32_t>(instanceBindings.size()), &shadowInstanceDescriptorSetLayout);
}

static void CreateShadowPass()
{
	CreateShadowDescriptorSetLayout();
	CreateShadowGraphicPipeline();

	CreatePerFrameBuffer(sizeof(SceneMatricesUniform), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&shadowSceneUniformBuffer);
}

void CleanupShadowPass()
{
	vkDestroyPipeline(g_vk.device, shadowPipeline, nullptr);
	vkDestroyPipelineLayout(g_vk.device, shadowPipelineLayout, nullptr);
	vkDestroyDescriptorSetLayout(g_vk.device, shadowDescriptorSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(g_vk.device, shadowInstanceDescriptorSetLayout, nullptr);

	DestroyPerFrameBuffer(&shadowSceneUniformBuffer);
}

void InitializeShadowPass(const RenderPass* renderpass, const Swapchain* swapchain)
{
	shadowRenderPass = renderpass;
	CreateShadowPass();
}

void ShadowRecordDrawCommandsBuffer(uint32_t currentFrame, const SceneFrameData* frameData, VkCommandBuffer graphicsCommandBuffer, VkExtent2D extent)
{
	CmdBeginShadowPass(graphicsCommandBuffer, currentFrame);
	for (size_t i = 0; i < frameData->renderableAssets.size(); ++i)
	{
		const SceneRenderableAsset* renderable = frameData->renderableAssets[i];
		CmdDrawShadowPass(graphicsCommandBuffer, renderable->descriptorSet, renderable->modelAsset, currentFrame);
	}
	CmdEndShadowPass(graphicsCommandBuffer);
}