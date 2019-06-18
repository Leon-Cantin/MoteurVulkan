#include "skybox.h"

#include "vk_shader.h"
#include "file_system.h"
#include "vk_buffer.h"
#include "descriptors.h"
#include "vk_debug.h"

#include <glm/gtc/matrix_transform.hpp>
#include <array>

VkDescriptorSetLayout skyboxDescriptorSetLayout;
VkPipelineLayout skyboxPipelineLayout;
const RenderPass* skyboxRenderPass;
VkPipeline skyboxGraphicsPipeline;
std::array<VkDescriptorSet, SIMULTANEOUS_FRAMES> skyboxDescriptorSets;
PerFrameBuffer skyboxUniformBuffer;

//TODO I want to use a mat3 but the mem requirement size is at 48 instead of 36, it ends up broken
// when received by the shader
// https://www.khronos.org/registry/vulkan/specs/1.0-extensions/html/vkspec.html#interfaces-resources  14.5.4
struct SkyboxUniformBufferObject {
	glm::mat4 inv_view_matrix;
};

void createSkyboxDescriptorSetLayout()
{
	const VkDescriptorSetLayoutBinding uboLayoutBinding = { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER , 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr };
	const VkDescriptorSetLayoutBinding samplerLayoutBinding = { 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };

	const std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };
	CreateDesciptorSetLayout(bindings.data(), static_cast<uint32_t>(bindings.size()), &skyboxDescriptorSetLayout);
}

void CreateSkyboxDescriptorSet(VkDescriptorPool descriptorPool, VkImageView skyboxImageView, VkSampler trilinearSampler)
{
	std::array<DescriptorSet, SIMULTANEOUS_FRAMES> descriptorSets;
	for (size_t i = 0; i < SIMULTANEOUS_FRAMES; ++i)
	{
		DescriptorSet& descriptorSet = descriptorSets[i] = {};
		descriptorSet.descriptors.resize(2);
		descriptorSet.descriptors[0] = { {skyboxUniformBuffer.buffers[i], 0, VK_WHOLE_SIZE}, {}, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0 };
		descriptorSet.descriptors[1] = { {}, {trilinearSampler, skyboxImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 };
		descriptorSet.layout = skyboxDescriptorSetLayout;

	}
	createDescriptorSets(descriptorPool, descriptorSets.size(), descriptorSets.data());
	for(uint32_t i = 0; i < SIMULTANEOUS_FRAMES; ++i)
		skyboxDescriptorSets[i] = descriptorSets[i].set;
}

static void create_skybox_graphics_pipeline(VkExtent2D extent)
{
	std::vector<char> vertShaderCode = FS::readFile("shaders/skybox.vert.spv");
	std::vector<char> fragShaderCode = FS::readFile("shaders/skybox.frag.spv");

	VkPipelineLayoutCreateInfo pipeline_layout_info = {};
	pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_info.setLayoutCount = 1; // Optional
	pipeline_layout_info.pSetLayouts = &skyboxDescriptorSetLayout; // Optional
	pipeline_layout_info.pushConstantRangeCount = 0; // Optional
	pipeline_layout_info.pPushConstantRanges = nullptr; // Optional

	if (vkCreatePipelineLayout(g_vk.device, &pipeline_layout_info, nullptr, &skyboxPipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}

	CreatePipeline(
		VK_NULL_HANDLE,
		0,
		VK_NULL_HANDLE,
		0,
		vertShaderCode, fragShaderCode,
		extent,
		skyboxRenderPass->vk_renderpass, 
		skyboxPipelineLayout,
		false,
		true,
		false,
		false,
		false,
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
		VK_COMPARE_OP_LESS_OR_EQUAL,
		&skyboxGraphicsPipeline);
}

void createSkyboxUniformBuffers()
{
	//TODO: do something so I can use mat3 instead of mat4
	CreatePerFrameBuffer(sizeof(SkyboxUniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&skyboxUniformBuffer);
}

void UpdateSkyboxUniformBuffers(size_t currentFrame, const glm::mat4& world_view_matrix)
{
	SkyboxUniformBufferObject subo = {};
	subo.inv_view_matrix = glm::mat3(glm::scale(transpose(world_view_matrix), glm::vec3(1.0f, -1.0f, 1.0f)));

	UpdatePerFrameBuffer( &skyboxUniformBuffer, &subo, sizeof(subo), currentFrame);
}

void CmdDrawSkybox(VkCommandBuffer commandBuffer, VkExtent2D extent, size_t currentFrame)
{
	CmdBeginVkLabel(commandBuffer, "Skybox Renderpass", glm::vec4(0.2f, 0.2f, 0.9f, 1.0f));
	BeginRenderPass(commandBuffer, *skyboxRenderPass, skyboxRenderPass->outputFrameBuffer[currentFrame].frameBuffer, extent);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, skyboxGraphicsPipeline);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, skyboxPipelineLayout, 0, 1, &skyboxDescriptorSets[currentFrame], 0, nullptr);

	vkCmdDraw(commandBuffer, 4, 1, 0, 0);
	EndRenderPass(commandBuffer);
	CmdEndVkLabel(commandBuffer);
}

void CleanupSkyboxAfterSwapchain()
{
	vkDestroyPipeline(g_vk.device, skyboxGraphicsPipeline, nullptr);
	vkDestroyPipelineLayout(g_vk.device, skyboxPipelineLayout, nullptr);
}

void CleanupSkybox()
{
	vkDestroyDescriptorSetLayout(g_vk.device, skyboxDescriptorSetLayout, nullptr);
	DestroyPerFrameBuffer(&skyboxUniformBuffer);
}

void ReloadSkyboxShaders(VkExtent2D extent)
{
	vkDestroyPipeline(g_vk.device, skyboxGraphicsPipeline, nullptr);
	create_skybox_graphics_pipeline(extent);
}

static void CreateSkyboxPipeline(const Swapchain& swapchain)
{
	createSkyboxDescriptorSetLayout();
	create_skybox_graphics_pipeline(swapchain.extent);
}

void RecreateSkyboxAfterSwapchain(const Swapchain* swapchain)
{
	create_skybox_graphics_pipeline(swapchain->extent);
}

void InitializeSkyboxRenderPass(const RenderPass* renderpass, const Swapchain* swapchain)
{
	skyboxRenderPass = renderpass;
	CreateSkyboxPipeline( *swapchain);
}

void SkyboxRecordDrawCommandsBuffer(uint32_t currentFrame, const SceneFrameData* frameData, VkCommandBuffer graphicsCommandBuffer, VkExtent2D extent)
{
	CmdDrawSkybox(graphicsCommandBuffer, extent, currentFrame);
}