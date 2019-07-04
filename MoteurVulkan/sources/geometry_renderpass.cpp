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

TechniqueDescriptorSetDesc geoPassSetDesc =
{
	{
		{ eTechniqueDataEntryName::SCENE_DATA, 0, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT },
		{ eTechniqueDataEntryName::LIGHT_DATA, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT },
	},
	2,
	{
		{ eTechniqueDataEntryImageName::ALBEDOS, 2, VK_SHADER_STAGE_FRAGMENT_BIT },
		{ eTechniqueDataEntryImageName::NORMALS, 3, VK_SHADER_STAGE_FRAGMENT_BIT },
		{ eTechniqueDataEntryImageName::SHADOWS, 4, VK_SHADER_STAGE_FRAGMENT_BIT },
	},
	3
};

TechniqueDescriptorSetDesc geoInstanceSetDesc =
{
	{
		{ eTechniqueDataEntryName::INSTANCE_DATA, 0, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT }
	},
	1
};

static void createGeoTechnique( VkExtent2D extent, Technique* technique )
{
	VkPushConstantRange pushConstantRange = {};
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof( uint32_t );
	pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	//Pipeline layout
	std::array<VkDescriptorSetLayout, 2> descriptorSetLayouts = { technique->renderpass_descriptor_layout, technique->instance_descriptor_layout };
	VkPipelineLayoutCreateInfo pipeline_layout_info = {};
	pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_info.setLayoutCount = 2;
	pipeline_layout_info.pSetLayouts = descriptorSetLayouts.data();
	pipeline_layout_info.pushConstantRangeCount = 1;
	pipeline_layout_info.pPushConstantRanges = &pushConstantRange;

	if( vkCreatePipelineLayout( g_vk.device, &pipeline_layout_info, nullptr, &technique->pipelineLayout ) != VK_SUCCESS ) {
		throw std::runtime_error( "failed to create pipeline layout!" );
	}
	
	VkVertexInputBindingDescription bindingDescriptions[5];
	VkVertexInputAttributeDescription attributeDescriptions[5];
	uint32_t bindingCount = GetBindingDescription( bindingDescriptions, attributeDescriptions );

	std::vector<char> vertShaderCode = FS::readFile("shaders/triangle.vert.spv");
	std::vector<char> fragShaderCode = FS::readFile("shaders/triangle.frag.spv");

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

	//TODO: build this outside
	VkDescriptorImageInfo albedos[] = { { sampler, textureView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
										{ sampler, textureView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
										{ sampler, textureView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
										{ sampler, textureView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
										{ sampler, textureView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL } };
	VkDescriptorImageInfo normalTextures[] = { { sampler, normalTextureView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL } };
	VkDescriptorImageInfo shadowTextures[] = { { shadowSampler, shadowTextureView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL } };

	std::array< InputBuffers, SIMULTANEOUS_FRAMES> inputBuffers;
	for( size_t i = 0; i < SIMULTANEOUS_FRAMES; ++i )
	{
		inputBuffers[i].data[static_cast< size_t >(eTechniqueDataEntryName::INSTANCE_DATA)] = &instanceUniformBuffers[i];
		inputBuffers[i].data[static_cast< size_t >(eTechniqueDataEntryName::SCENE_DATA)] = &sceneUniformBuffers[i];
		inputBuffers[i].data[static_cast< size_t >(eTechniqueDataEntryName::LIGHT_DATA)] = &lightBuffers[i];


		inputBuffers[i].dataImages[static_cast< size_t >(eTechniqueDataEntryImageName::ALBEDOS)] = albedos;
		inputBuffers[i].dataImages[static_cast< size_t >(eTechniqueDataEntryImageName::NORMALS)] = normalTextures;
		inputBuffers[i].dataImages[static_cast< size_t >(eTechniqueDataEntryImageName::SHADOWS)] = shadowTextures;
	}

	for( size_t i = 0; i < SIMULTANEOUS_FRAMES; ++i )
	{
		CreateDescriptorSet( &inputBuffers[i], &geoPassSetDesc, technique->renderpass_descriptor_layout, descriptorPool, &technique->renderPass_descriptor[i] );
		CreateDescriptorSet( &inputBuffers[i], &geoInstanceSetDesc, technique->instance_descriptor_layout, descriptorPool, &technique->instance_descriptor[i] );
	}
}


void createGeoDescriptorSetLayout( Technique * technique )
{
	CreateDescriptorSetLayout( &geoPassSetDesc, &technique->renderpass_descriptor_layout );
	CreateDescriptorSetLayout( &geoInstanceSetDesc, &technique->instance_descriptor_layout );
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
