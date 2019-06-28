#include "geometry_renderpass.h"
#include "..\shaders\shadersCommon.h"

#include "file_system.h"
#include "descriptors.h"
#include "vk_commands.h"
#include "vk_debug.h"
#include "material.h"

#include <vector>

const RenderPass* geometryRenderPass;
GfxMaterial m_geoMaterial;

static void createGeoTechnique( VkExtent2D extent, Technique* technique )
{
	VkVertexInputBindingDescription bindingDescriptions[5];
	VkVertexInputAttributeDescription attributeDescriptions[5];
	uint32_t bindingCount = GetBindingDescription( bindingDescriptions, attributeDescriptions );

	std::vector<char> vertShaderCode = FS::readFile("shaders/triangle.vert.spv");
	std::vector<char> fragShaderCode = FS::readFile("shaders/triangle.frag.spv");

	VkPushConstantRange pushConstantRange = {};
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(uint32_t);
	pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	//Pipeline layout
	std::array<VkDescriptorSetLayout, 2> descriptorSetLayouts = { technique->renderpass_descriptor_layout, technique->instance_descriptor_layout };
	VkPipelineLayoutCreateInfo pipeline_layout_info = {};
	pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_info.setLayoutCount = 2;
	pipeline_layout_info.pSetLayouts = descriptorSetLayouts.data();
	pipeline_layout_info.pushConstantRangeCount = 1;
	pipeline_layout_info.pPushConstantRanges = &pushConstantRange;

	if (vkCreatePipelineLayout(g_vk.device, &pipeline_layout_info, nullptr, &technique->pipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}

	VICreation viState = { bindingDescriptions, bindingCount, attributeDescriptions, bindingCount};
	//TODO: these cast are dangerous for alligment
	std::vector<ShaderCreation> shaderState = { 
		{ reinterpret_cast< uint32_t* >(vertShaderCode.data()), vertShaderCode.size(), "main", VK_SHADER_STAGE_VERTEX_BIT },
		{ reinterpret_cast< uint32_t* >(fragShaderCode.data()), fragShaderCode.size(), "main", VK_SHADER_STAGE_FRAGMENT_BIT } };
	RasterizationState rasterizationState;
	rasterizationState.backFaceCulling = true;
	rasterizationState.depthBiased = false;
	DepthStencilState depthStencilState;
	depthStencilState.depthRead = true;
	depthStencilState.depthWrite = true;
	depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS;

	CreatePipeline( viState,
		shaderState,
		extent,
		geometryRenderPass->vk_renderpass,
		technique->pipelineLayout,
		rasterizationState,
		depthStencilState,
		false,
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		&technique->pipeline);
}

void CreateGeometryDescriptorSet( VkDescriptorPool descriptorPool, VkBuffer* sceneUniformBuffers, VkBuffer* instanceUniformBuffers, VkBuffer* lightBuffers, VkImageView textureView,
	VkImageView normalTextureView, VkSampler sampler, VkImageView shadowTextureView, VkSampler shadowSampler )
{
	Technique* technique = &m_geoMaterial.techniques[0];
	std::array<DescriptorSet2, SIMULTANEOUS_FRAMES> descriptorSets2;

	VkDescriptorBufferInfo sceneUbos[] = { {sceneUniformBuffers[0], 0, VK_WHOLE_SIZE}, {sceneUniformBuffers[1], 0, VK_WHOLE_SIZE} };
	VkDescriptorBufferInfo lightUbos[] = { { lightBuffers[0], 0, VK_WHOLE_SIZE }, { lightBuffers[1], 0, VK_WHOLE_SIZE } };

	//TODO fix this mess
	VkDescriptorImageInfo albedos[] = { { sampler, textureView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
	{ sampler, textureView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
	{ sampler, textureView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
	{ sampler, textureView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
	{ sampler, textureView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL } };
	VkDescriptorImageInfo normalTextures[] = { { sampler, normalTextureView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL } };
	VkDescriptorImageInfo shadowTextures[] = { { shadowSampler, shadowTextureView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL } };
	//Per render pass descriptor est
	for (size_t i = 0; i < SIMULTANEOUS_FRAMES; ++i)
	{
		DescriptorSet2& geoDescriptorSet = descriptorSets2[i] = {};
		geoDescriptorSet.descriptors.resize(5);
		geoDescriptorSet.descriptors[0] = { &sceneUbos[i], {}, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0 };
		geoDescriptorSet.descriptors[1] = { &lightUbos[i], {}, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 };
		geoDescriptorSet.descriptors[2] = { {}, albedos, 5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2 };
		geoDescriptorSet.descriptors[3] = { {}, normalTextures, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3 };
		geoDescriptorSet.descriptors[4] = { {}, shadowTextures, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4 };
		geoDescriptorSet.layout = technique->renderpass_descriptor_layout;
	}
	createDescriptorSets2(descriptorPool, descriptorSets2.size(), descriptorSets2.data());

	for (size_t i = 0; i < SIMULTANEOUS_FRAMES; ++i)
		technique->renderPass_descriptor[i] = descriptorSets2[i].set;

	std::array<DescriptorSet, SIMULTANEOUS_FRAMES> descriptorSets;
	//Per instance descriptor set
	for (size_t i = 0; i < SIMULTANEOUS_FRAMES; ++i)
	{
		DescriptorSet& geoDescriptorSet = descriptorSets[i] = {};
		geoDescriptorSet.descriptors.resize(1);
		geoDescriptorSet.descriptors[0] = { {instanceUniformBuffers[i], 0, VK_WHOLE_SIZE}, {}, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 0 };
		geoDescriptorSet.layout = technique->instance_descriptor_layout;
	}
	createDescriptorSets(descriptorPool, descriptorSets.size(), descriptorSets.data());

	for (size_t i = 0; i < SIMULTANEOUS_FRAMES; ++i)
		technique->instance_descriptor[i] = descriptorSets[i].set;
}

void createGeoDescriptorSetLayout( Technique * technique )
{
	const VkDescriptorSetLayoutBinding uboLayoutBinding = { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER , 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
	const VkDescriptorSetLayoutBinding lightLayoutBinding = { 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
	const VkDescriptorSetLayoutBinding samplerLayoutBinding = { 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 5, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };	
	const VkDescriptorSetLayoutBinding normalSamplerLayoutBinding = { 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
	const VkDescriptorSetLayoutBinding shadowSamplerLayoutBinding = { 4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };

	const std::array<VkDescriptorSetLayoutBinding, 5> bindings = { uboLayoutBinding, samplerLayoutBinding, lightLayoutBinding, normalSamplerLayoutBinding, shadowSamplerLayoutBinding };
	CreateDesciptorSetLayout(bindings.data(), static_cast<uint32_t>(bindings.size()), &technique->renderpass_descriptor_layout );

	const VkDescriptorSetLayoutBinding instanceUboLayoutBinding = { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC , 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
	const std::array<VkDescriptorSetLayoutBinding, 1> bindings2 = { instanceUboLayoutBinding };
	CreateDesciptorSetLayout(bindings2.data(), static_cast<uint32_t>(bindings2.size()), &technique->instance_descriptor_layout );
}

void CreateGeometryPipeline(const Swapchain& swapchain)
{
	createGeoDescriptorSetLayout( &m_geoMaterial.techniques[0] );
	createGeoTechnique( swapchain.extent, &m_geoMaterial.techniques[0] );
}

void CmdBeginGeometryRenderPass(VkCommandBuffer commandBuffer, VkExtent2D extent, uint32_t currentFrame)
{
	CmdBeginVkLabel(commandBuffer, "Geometry renderpass", glm::vec4(0.8f, 0.6f, 0.4f, 1.0f));
	BeginRenderPass(commandBuffer, *geometryRenderPass, geometryRenderPass->outputFrameBuffer[currentFrame].frameBuffer, extent);

	BeginTechnique( commandBuffer, &m_geoMaterial.techniques[0], currentFrame );
}

void CmdEndGeometryRenderPass(VkCommandBuffer vkCommandBuffer)
{
	EndRenderPass(vkCommandBuffer);
	CmdEndVkLabel(vkCommandBuffer);
}

static void CmdDrawModelAsset( VkCommandBuffer commandBuffer, const SceneRenderableAsset* renderableAsset, uint32_t currentFrame)
{	
	const Technique* technique = &m_geoMaterial.techniques[0];
	//TODO: could do like the VIB, query a texture of X from an array using an enum index
	//Have a list of all required paremeters for this pass.
	vkCmdPushConstants( commandBuffer, technique->pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof( uint32_t ), &renderableAsset->albedoIndex );

	const SceneInstanceSet* instanceSet = renderableAsset->descriptorSet;
	const GfxModel* modelAsset = renderableAsset->modelAsset;
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, technique->pipelineLayout, INSTANCE_SET, 1,
		&technique->instance_descriptor[currentFrame], 1, &instanceSet->geometryBufferOffsets[currentFrame]);
	CmdDrawIndexed(commandBuffer, *modelAsset);	
}

void ReloadGeometryShaders( VkExtent2D extent )
{
	Technique* technique = &m_geoMaterial.techniques[0];
	vkDestroyPipeline(g_vk.device, technique->pipeline, nullptr);
	createGeoTechnique(extent, technique);
}

void CleanupGeometryRenderpassAfterSwapchain()
{
	Technique* technique = &m_geoMaterial.techniques[0];
	vkDestroyPipeline(g_vk.device, technique->pipeline, nullptr);
	vkDestroyPipelineLayout(g_vk.device, technique->pipelineLayout, nullptr);
	//TODO: format could change after a swap chain recreation. The renderpas depends on it.
}

void CleanupGeometryRenderpass()
{
	Technique* technique = &m_geoMaterial.techniques[0];
	vkDestroyDescriptorSetLayout(g_vk.device, technique->renderpass_descriptor_layout, nullptr);
	vkDestroyDescriptorSetLayout(g_vk.device, technique->instance_descriptor_layout, nullptr);
}

void InitializeGeometryRenderPass(const RenderPass* renderpass, const Swapchain* swapchain)
{
	geometryRenderPass = renderpass;
	CreateGeometryPipeline(*swapchain);
}

void RecreateGeometryAfterSwapChain(const Swapchain* swapchain)
{
	createGeoTechnique(swapchain->extent, &m_geoMaterial.techniques[0] );
}

void GeometryRecordDrawCommandsBuffer(uint32_t currentFrame, const SceneFrameData* frameData, VkCommandBuffer graphicsCommandBuffer, VkExtent2D extent)
{
	CmdBeginGeometryRenderPass(graphicsCommandBuffer, extent, currentFrame);
	for (size_t i = 0; i < frameData->renderableAssets.size(); ++i)
	{
		const SceneRenderableAsset* renderable = frameData->renderableAssets[i];
		CmdDrawModelAsset(graphicsCommandBuffer, renderable, currentFrame);
	}
	CmdEndGeometryRenderPass(graphicsCommandBuffer);
}
