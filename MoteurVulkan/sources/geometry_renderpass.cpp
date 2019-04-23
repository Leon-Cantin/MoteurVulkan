#include "geometry_renderpass.h"
#include "..\shaders\shadersCommon.h"

#include "vk_framework.h"
#include "descriptors.h"
#include "vk_commands.h"
#include "vk_debug.h"

#include <vector>

VkDescriptorSetLayout geoDescriptorSetLayout;
VkDescriptorSetLayout geoInstanceDescriptorSetLayout;
VkPipelineLayout geoPipelineLayout;
const RenderPass* geometryRenderPass;
VkPipeline geoGraphicsPipeline;

std::array<VkDescriptorSet, SIMULTANEOUS_FRAMES> geoDescriptorSets;
std::array<VkDescriptorSet, SIMULTANEOUS_FRAMES> geoInstanceDescriptorSet;

void createGeoGraphicPipeline( VkExtent2D extent )
{
	auto bindingDescription = Vertex::get_binding_description();
	auto attributeDescriptions = Vertex::get_attribute_descriptions();

	std::vector<char> vertShaderCode = readFile("shaders/triangle.vert.spv");
	std::vector<char> fragShaderCode = readFile("shaders/triangle.frag.spv");

	//Pipeline layout
	std::array<VkDescriptorSetLayout, 2> descriptorSetLayouts = { geoDescriptorSetLayout, geoInstanceDescriptorSetLayout };
	VkPipelineLayoutCreateInfo pipeline_layout_info = {};
	pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_info.setLayoutCount = 2;
	pipeline_layout_info.pSetLayouts = descriptorSetLayouts.data();
	pipeline_layout_info.pushConstantRangeCount = 0;
	pipeline_layout_info.pPushConstantRanges = nullptr;

	if (vkCreatePipelineLayout(g_vk.device, &pipeline_layout_info, nullptr, &geoPipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}

	CreatePipeline(&bindingDescription,
		attributeDescriptions.data(),
		static_cast<uint32_t>(attributeDescriptions.size()),
		vertShaderCode, fragShaderCode,
		extent,
		geometryRenderPass->vk_renderpass,
		geoPipelineLayout,
		false,
		true,
		true,
		false,
		true,
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		VK_COMPARE_OP_LESS,
		&geoGraphicsPipeline);
}

void AddGeometryRenderPass(const RenderPass* renderpass)
{
	geometryRenderPass = renderpass;
}

void CreateGeometryDescriptorSet(VkDescriptorPool descriptorPool, VkBuffer* sceneUniformBuffers, VkBuffer* instanceUniformBuffers, VkBuffer* lightBuffers, VkImageView textureView,
	VkImageView normalTextureView, VkSampler sampler, VkImageView shadowTextureView, VkSampler shadowSampler)
{
	std::array<DescriptorSet, SIMULTANEOUS_FRAMES> descriptorSets;

	//Per render pass descriptor est
	for (size_t i = 0; i < SIMULTANEOUS_FRAMES; ++i)
	{
		DescriptorSet& geoDescriptorSet = descriptorSets[i] = {};
		geoDescriptorSet.descriptors.resize(5);
		geoDescriptorSet.descriptors[0] = { {sceneUniformBuffers[i], 0, VK_WHOLE_SIZE}, {}, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0 };
		geoDescriptorSet.descriptors[1] = { {lightBuffers[i], 0, VK_WHOLE_SIZE}, {}, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 };
		geoDescriptorSet.descriptors[2] = { {},  { sampler, textureView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2 };
		geoDescriptorSet.descriptors[3] = { {},  { sampler, normalTextureView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3 };
		geoDescriptorSet.descriptors[4] = { {},  { shadowSampler, shadowTextureView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4 };
		geoDescriptorSet.layout = geoDescriptorSetLayout;
	}
	createDescriptorSets(descriptorPool, descriptorSets.size(), descriptorSets.data());

	for (size_t i = 0; i < SIMULTANEOUS_FRAMES; ++i)
		geoDescriptorSets[i] = descriptorSets[i].set;

	//Per instance descriptor set
	for (size_t i = 0; i < SIMULTANEOUS_FRAMES; ++i)
	{
		DescriptorSet& geoDescriptorSet = descriptorSets[i] = {};
		geoDescriptorSet.descriptors.resize(1);
		geoDescriptorSet.descriptors[0] = { {instanceUniformBuffers[i], 0, VK_WHOLE_SIZE}, {}, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 0 };
		geoDescriptorSet.layout = geoInstanceDescriptorSetLayout;
	}
	createDescriptorSets(descriptorPool, descriptorSets.size(), descriptorSets.data());

	for (size_t i = 0; i < SIMULTANEOUS_FRAMES; ++i)
		geoInstanceDescriptorSet[i] = descriptorSets[i].set;
}

void createGeoDescriptorSetLayout()
{
	const VkDescriptorSetLayoutBinding uboLayoutBinding = { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER , 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
	const VkDescriptorSetLayoutBinding lightLayoutBinding = { 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
	const VkDescriptorSetLayoutBinding samplerLayoutBinding = { 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };	
	const VkDescriptorSetLayoutBinding normalSamplerLayoutBinding = { 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
	const VkDescriptorSetLayoutBinding shadowSamplerLayoutBinding = { 4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };

	const std::array<VkDescriptorSetLayoutBinding, 5> bindings = { uboLayoutBinding, samplerLayoutBinding, lightLayoutBinding, normalSamplerLayoutBinding, shadowSamplerLayoutBinding };
	CreateDesciptorSetLayout(bindings.data(), static_cast<uint32_t>(bindings.size()), &geoDescriptorSetLayout);

	const VkDescriptorSetLayoutBinding instanceUboLayoutBinding = { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC , 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
	const std::array<VkDescriptorSetLayoutBinding, 1> bindings2 = { instanceUboLayoutBinding };
	CreateDesciptorSetLayout(bindings2.data(), static_cast<uint32_t>(bindings2.size()), &geoInstanceDescriptorSetLayout);
}

void CmdBeginGeometryRenderPass(VkCommandBuffer commandBuffer, VkExtent2D extent, uint32_t currentFrame)
{
	CmdBeginVkLabel(commandBuffer, "Geometry renderpass", glm::vec4(0.8f, 0.6f, 0.4f, 1.0f));
	BeginRenderPass(commandBuffer, *geometryRenderPass, geometryRenderPass->outputFrameBuffer[currentFrame].frameBuffer, extent);
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, geoGraphicsPipeline);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, geoPipelineLayout, RENDERPASS_SET, 1, &geoDescriptorSets[currentFrame], 0, nullptr);
}

void CmdEndGeometryRenderPass(VkCommandBuffer vkCommandBuffer)
{
	EndRenderPass(vkCommandBuffer);
	CmdEndVkLabel(vkCommandBuffer);
}

void CmdDrawModelAsset( VkCommandBuffer commandBuffer, const SceneInstanceSet* instanceSet, const ModelAsset& modelAsset, uint32_t currentFrame)
{
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, geoPipelineLayout, INSTANCE_SET, 1,
		&geoInstanceDescriptorSet[currentFrame], 1, &instanceSet->geometryBufferOffsets[currentFrame]);
	CmdDrawIndexed(commandBuffer, modelAsset);	
}

void ReloadGeometryShaders( VkExtent2D extent )
{
	vkDestroyPipeline(g_vk.device, geoGraphicsPipeline, nullptr);
	createGeoGraphicPipeline(extent);
}

void CleanupGeometryRenderpassAfterSwapchain()
{
	vkDestroyPipeline(g_vk.device, geoGraphicsPipeline, nullptr);
	vkDestroyPipelineLayout(g_vk.device, geoPipelineLayout, nullptr);
	//TODO: format could change after a swap chain recreation. The renderpas depends on it.
}

void CleanupGeometryRenderpass()
{
	vkDestroyDescriptorSetLayout(g_vk.device, geoDescriptorSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(g_vk.device, geoInstanceDescriptorSetLayout, nullptr);
}
