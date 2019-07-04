#include "skybox.h"

#include "vk_shader.h"
#include "file_system.h"
#include "vk_buffer.h"
#include "descriptors.h"
#include "vk_debug.h"
#include "material.h"

#include <glm/gtc/matrix_transform.hpp>
#include <array>

GfxMaterial skyboxMaterial;
const RenderPass* skyboxRenderPass;
PerFrameBuffer skyboxUniformBuffer;

//TODO I want to use a mat3 but the mem requirement size is at 48 instead of 36, it ends up broken
// when received by the shader
// https://www.khronos.org/registry/vulkan/specs/1.0-extensions/html/vkspec.html#interfaces-resources  14.5.4
struct SkyboxUniformBufferObject {
	glm::mat4 inv_view_matrix;
};

static void createSkyboxDescriptorSetLayout(Technique* technique)
{
	const VkDescriptorSetLayoutBinding uboLayoutBinding = { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER , 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr };
	const VkDescriptorSetLayoutBinding samplerLayoutBinding = { 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };

	const std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };
	CreateDesciptorSetLayout(bindings.data(), static_cast<uint32_t>(bindings.size()), &technique->renderpass_descriptor_layout);
}

void CreateSkyboxDescriptorSet(VkDescriptorPool descriptorPool, VkImageView skyboxImageView, VkSampler trilinearSampler)
{
	Technique* technique = &skyboxMaterial.techniques[0];

	std::array<DescriptorSet, SIMULTANEOUS_FRAMES> descriptorSets;
	for (size_t i = 0; i < SIMULTANEOUS_FRAMES; ++i)
	{
		DescriptorSet& descriptorSet = descriptorSets[i] = {};
		descriptorSet.descriptors.resize(2);
		descriptorSet.descriptors[0] = { {skyboxUniformBuffer.buffers[i], 0, VK_WHOLE_SIZE}, {}, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0 };
		descriptorSet.descriptors[1] = { {}, {trilinearSampler, skyboxImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 };
		descriptorSet.layout = technique->renderpass_descriptor_layout;

	}
	createDescriptorSets(descriptorPool, descriptorSets.size(), descriptorSets.data());
	for(uint32_t i = 0; i < SIMULTANEOUS_FRAMES; ++i)
		technique->renderPass_descriptor[i] = descriptorSets[i].set;
}

static void CreateSkyboxTechnique(VkExtent2D extent, Technique* technique)
{
	VkPipelineLayoutCreateInfo pipeline_layout_info = {};
	pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_info.setLayoutCount = 1; // Optional
	pipeline_layout_info.pSetLayouts = &technique->renderpass_descriptor_layout; // Optional
	pipeline_layout_info.pushConstantRangeCount = 0; // Optional
	pipeline_layout_info.pPushConstantRanges = nullptr; // Optional

	if( vkCreatePipelineLayout( g_vk.device, &pipeline_layout_info, nullptr, &technique->pipelineLayout ) != VK_SUCCESS ) {
		throw std::runtime_error( "failed to create pipeline layout!" );
	}

	std::vector<char> vertShaderCode = FS::readFile("shaders/skybox.vert.spv");
	std::vector<char> fragShaderCode = FS::readFile("shaders/skybox.frag.spv");

	VICreation viState = { VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 0 };
	//TODO: these cast are dangerous for alligment
	std::vector<ShaderCreation> shaderState = {
		{ reinterpret_cast< uint32_t* >(vertShaderCode.data()), vertShaderCode.size(), "main", VK_SHADER_STAGE_VERTEX_BIT },
		{ reinterpret_cast< uint32_t* >(fragShaderCode.data()), fragShaderCode.size(), "main", VK_SHADER_STAGE_FRAGMENT_BIT } };
	RasterizationState rasterizationState;
	rasterizationState.backFaceCulling = false;
	rasterizationState.depthBiased = false;
	DepthStencilState depthStencilState;
	depthStencilState.depthRead = true;
	depthStencilState.depthWrite = false;
	depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

	CreatePipeline(
		viState,
		shaderState,
		extent,
		skyboxRenderPass->vk_renderpass, 
		technique->pipelineLayout,
		rasterizationState,
		depthStencilState,
		false,
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
		&technique->pipeline );
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
	Technique* technique = &skyboxMaterial.techniques[0];
	CmdBeginVkLabel( commandBuffer, "Skybox Renderpass", glm::vec4( 0.2f, 0.2f, 0.9f, 1.0f ) );
	BeginRenderPass( commandBuffer, *skyboxRenderPass, skyboxRenderPass->outputFrameBuffer[currentFrame].frameBuffer, extent );

	vkCmdBindPipeline( commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, technique->pipeline );
	vkCmdBindDescriptorSets( commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, technique->pipelineLayout, 0, 1, &technique->renderPass_descriptor[currentFrame], 0, nullptr );

	vkCmdDraw(commandBuffer, 4, 1, 0, 0);
	EndRenderPass(commandBuffer);
	CmdEndVkLabel(commandBuffer);
}

//TODO: Probably just destroy everything and rebuild all pipeline objects
void CleanupSkyboxAfterSwapchain()
{
	Technique* technique = &skyboxMaterial.techniques[0];
	vkDestroyPipeline(g_vk.device, technique->pipeline, nullptr);
	vkDestroyPipelineLayout(g_vk.device, technique->pipelineLayout, nullptr);
}

void CleanupSkybox()
{
	Technique* technique = &skyboxMaterial.techniques[0];
	vkDestroyDescriptorSetLayout(g_vk.device, technique->renderpass_descriptor_layout, nullptr);
	DestroyPerFrameBuffer(&skyboxUniformBuffer);
}

void ReloadSkyboxShaders(VkExtent2D extent)
{
	Technique* technique = &skyboxMaterial.techniques[0];
	vkDestroyPipeline(g_vk.device, technique->pipeline, nullptr);
	CreateSkyboxTechnique(extent, technique);
}

static void CreateSkyboxPipeline(const Swapchain& swapchain)
{
	Technique* technique = &skyboxMaterial.techniques[0];
	createSkyboxDescriptorSetLayout( technique );
	CreateSkyboxTechnique( swapchain.extent, technique );
}

void RecreateSkyboxAfterSwapchain(const Swapchain* swapchain)
{
	Technique* technique = &skyboxMaterial.techniques[0];
	CreateSkyboxTechnique( swapchain->extent, technique );
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