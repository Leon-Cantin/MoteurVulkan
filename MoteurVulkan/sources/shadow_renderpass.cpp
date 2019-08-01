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
#include "material.h"

#include <vector>
#include <array>

const RenderPass* shadowRenderPass;

//TODO: this is redundant, also found in frame graph
constexpr VkFormat RT_FORMAT_SHADOW_DEPTH = VK_FORMAT_D32_SFLOAT;
constexpr VkExtent2D RT_EXTENT_SHADOW = { 1024, 1024 };

PerFrameBuffer shadowSceneUniformBuffer;
GfxMaterial shadowMaterial;

TechniqueDescriptorSetDesc shadowPassSet =
{
	{
		{ eTechniqueDataEntryName::SHADOW_DATA, 0, VK_SHADER_STAGE_VERTEX_BIT }
	},
	1
};

TechniqueDescriptorSetDesc shadowInstanceSet =
{
	{
		{ eTechniqueDataEntryName::INSTANCE_DATA, 0, VK_SHADER_STAGE_VERTEX_BIT }
	},
	1
};

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

static void CreateDescritorSetLayout( Technique* technique )
{
	CreateDescriptorSetLayout( &shadowPassSet, &technique->renderpass_descriptor_layout );
	CreateDescriptorSetLayout( &shadowInstanceSet, &technique->instance_descriptor_layout );
}

static void CreateShadowTechnique( const RenderPass* renderpass, Technique* technique )
{
	//Create pipeline layout
	VkDescriptorSetLayout descriptorSetLayouts[] = { technique->renderpass_descriptor_layout, technique->instance_descriptor_layout };
	VkPipelineLayoutCreateInfo pipeline_layout_info = {};
	pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_info.setLayoutCount = 2; // Optional
	pipeline_layout_info.pSetLayouts = descriptorSetLayouts; // Optional
	pipeline_layout_info.pushConstantRangeCount = 0; // Optional
	pipeline_layout_info.pPushConstantRanges = nullptr; // Optional

	if( vkCreatePipelineLayout( g_vk.device, &pipeline_layout_info, nullptr, &technique->pipelineLayout ) != VK_SUCCESS ) {
		throw std::runtime_error( "failed to create pipeline layout!" );
	}

	//Create the PSO
	VkVertexInputBindingDescription bindingDescriptions[5];
	VkVertexInputAttributeDescription attributeDescriptions[5];
	uint32_t bindingCount = GetBindingDescription( bindingDescriptions, attributeDescriptions );

	std::vector<char> vertShaderCode = FS::readFile( "shaders/shadows.vert.spv" );

	VICreation viState = { bindingDescriptions, bindingCount, attributeDescriptions, bindingCount };
	//TODO: these cast are dangerous for alligment
	std::vector<ShaderCreation> shaderState = {
		{ reinterpret_cast< uint32_t* >(vertShaderCode.data()), vertShaderCode.size(), "main", VK_SHADER_STAGE_VERTEX_BIT } };
	RasterizationState rasterizationState;
	rasterizationState.backFaceCulling = true;
	rasterizationState.depthBiased = true;
	DepthStencilState depthStencilState;
	depthStencilState.depthRead = true;
	depthStencilState.depthWrite = true;
	depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS;

	CreatePipeline( viState,
		shaderState,
		RT_EXTENT_SHADOW,
		renderpass->vk_renderpass,
		technique->pipelineLayout,
		rasterizationState,
		depthStencilState,
		false,
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		&technique->pipeline );
}

void CreateShadowDescriptorSet(VkDescriptorPool descriptorPool, VkBuffer*instanceUniformBuffer)
{
	//TODO: build this outside
	std::array< InputBuffers, SIMULTANEOUS_FRAMES> inputBuffers;
	for( size_t i = 0; i < SIMULTANEOUS_FRAMES; ++i )
	{
		inputBuffers[i].data[static_cast< size_t >(eTechniqueDataEntryName::INSTANCE_DATA)] = &instanceUniformBuffer[i];
		inputBuffers[i].data[static_cast< size_t >(eTechniqueDataEntryName::SHADOW_DATA)] = &shadowSceneUniformBuffer.buffers[i];
	}

	Technique* technique = &shadowMaterial.techniques[0];
	for (size_t i = 0; i < SIMULTANEOUS_FRAMES; ++i)
	{
		CreateDescriptorSet( &inputBuffers[i], &shadowPassSet, technique->renderpass_descriptor_layout, descriptorPool, &technique->renderPass_descriptor[i] );
		CreateDescriptorSet( &inputBuffers[i], &shadowInstanceSet, technique->instance_descriptor_layout, descriptorPool, &technique->instance_descriptor[i] );
	}
}

static void CreateShadowPass()
{
	CreateDescritorSetLayout( &shadowMaterial.techniques[0] );
	CreateShadowTechnique( shadowRenderPass, &shadowMaterial.techniques[0] );

	CreatePerFrameBuffer(sizeof(SceneMatricesUniform), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&shadowSceneUniformBuffer);
}

void CleanupShadowPass()
{
	Destroy( &shadowMaterial );
	DestroyPerFrameBuffer(&shadowSceneUniformBuffer);
}

void InitializeShadowPass(const RenderPass* renderpass, const Swapchain* swapchain)
{
	shadowRenderPass = renderpass;
	CreateShadowPass();
}

/*
	Draw stuff
*/

static void CmdBeginShadowPass( VkCommandBuffer commandBuffer, size_t currentFrame )
{
	CmdBeginVkLabel( commandBuffer, "Shadow Renderpass", glm::vec4( 0.5f, 0.2f, 0.4f, 1.0f ) );
	BeginRenderPass( commandBuffer, *shadowRenderPass, shadowRenderPass->frameBuffer.frameBuffer, shadowRenderPass->frameBuffer.extent );

	BeginTechnique( commandBuffer, &shadowMaterial.techniques[0], currentFrame );
}

static void CmdDrawModel( VkCommandBuffer commandBuffer, const SceneInstanceSet* instanceSet, const GfxModel* modelAsset, uint32_t currentFrame )
{
	const Technique* technique = &shadowMaterial.techniques[0];
	vkCmdBindDescriptorSets( commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, technique->pipelineLayout, INSTANCE_SET, 1,
		&technique->instance_descriptor[currentFrame], 1, &instanceSet->geometryBufferOffsets[currentFrame] );
	CmdDrawIndexed( commandBuffer, *modelAsset );
}

static void CmdEndShadowPass( VkCommandBuffer commandBuffer )
{
	EndRenderPass( commandBuffer );
	CmdEndVkLabel( commandBuffer );
}

void ShadowRecordDrawCommandsBuffer(uint32_t currentFrame, const SceneFrameData* frameData, VkCommandBuffer graphicsCommandBuffer, VkExtent2D extent)
{
	CmdBeginShadowPass(graphicsCommandBuffer, currentFrame);
	for (size_t i = 0; i < frameData->renderableAssets.size(); ++i)
	{
		const SceneRenderableAsset* renderable = frameData->renderableAssets[i];
		CmdDrawModel(graphicsCommandBuffer, renderable->descriptorSet, renderable->modelAsset, currentFrame);
	}
	CmdEndShadowPass(graphicsCommandBuffer);
}