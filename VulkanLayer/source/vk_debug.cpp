#include "vk_debug.h"

#include <stdexcept>
#include <iostream>

PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugMarkerSetObjectName_func = VK_NULL_HANDLE;

PFN_vkCmdBeginDebugUtilsLabelEXT vkCmdBeginDebugUtilsLabelEXT_func = VK_NULL_HANDLE;
PFN_vkCmdEndDebugUtilsLabelEXT vkCmdEndDebugUtilsLabelEXT_func = VK_NULL_HANDLE;
PFN_vkCmdInsertDebugUtilsLabelEXT vkCmdInsertDebugUtilsLabelEXT_func = VK_NULL_HANDLE;

PFN_vkQueueBeginDebugUtilsLabelEXT vkQueueBeginDebugUtilsLabelEXT_func = VK_NULL_HANDLE;
PFN_vkQueueEndDebugUtilsLabelEXT vkQueueEndDebugUtilsLabelEXT_func = VK_NULL_HANDLE;
PFN_vkQueueInsertDebugUtilsLabelEXT vkQueueInsertDebugUtilsLabelEXT_func = VK_NULL_HANDLE;

VkDebugUtilsMessengerEXT debug_callback_messenger;

template<class T>
void AcquireDebugUtilsFunc(T* pFn, const char * funcName)
{
	*pFn = (T)vkGetInstanceProcAddr(g_vk.vk_instance, funcName);
	if (*pFn == VK_NULL_HANDLE)
	{
		char buffer[256];
		sprintf_s(buffer, 256, "Couldn't acquire %s", funcName);
		throw std::runtime_error(buffer);
	}
}

void MarkVkObject(uint64_t objectHandle, VkObjectType objetType, const char * name)
{
	if (vkSetDebugMarkerSetObjectName_func == VK_NULL_HANDLE)
		AcquireDebugUtilsFunc< PFN_vkSetDebugUtilsObjectNameEXT>(&vkSetDebugMarkerSetObjectName_func, "vkSetDebugUtilsObjectNameEXT");

	VkDebugUtilsObjectNameInfoEXT info = {};
	info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
	info.objectHandle = objectHandle;
	info.objectType = objetType;
	info.pObjectName = name;
	vkSetDebugMarkerSetObjectName_func(g_vk.device, &info);
}

void CmdBeginVkLabel(VkCommandBuffer commandBuffer, const char * name, const glm::vec4& color)
{
	if (vkCmdBeginDebugUtilsLabelEXT_func == VK_NULL_HANDLE)
		AcquireDebugUtilsFunc< PFN_vkCmdBeginDebugUtilsLabelEXT>(&vkCmdBeginDebugUtilsLabelEXT_func, "vkCmdBeginDebugUtilsLabelEXT");

	VkDebugUtilsLabelEXT info = {};
	info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
	info.pLabelName = name;
	info.color[0] = color.r;
	info.color[1] = color.g;
	info.color[2] = color.b;
	info.color[3] = color.a;
	vkCmdBeginDebugUtilsLabelEXT_func(commandBuffer, &info);
}

void CmdEndVkLabel(VkCommandBuffer commandBuffer)
{
	if (vkCmdEndDebugUtilsLabelEXT_func == VK_NULL_HANDLE)
		AcquireDebugUtilsFunc< PFN_vkCmdEndDebugUtilsLabelEXT>(&vkCmdEndDebugUtilsLabelEXT_func, "vkCmdEndDebugUtilsLabelEXT");

	vkCmdEndDebugUtilsLabelEXT_func(commandBuffer);
}

void CmdInsertVkLabel(VkCommandBuffer commandBuffer, const char * name, const glm::vec4& color)
{
	if (vkCmdInsertDebugUtilsLabelEXT_func == VK_NULL_HANDLE)
		AcquireDebugUtilsFunc< PFN_vkCmdInsertDebugUtilsLabelEXT>(&vkCmdInsertDebugUtilsLabelEXT_func, "vkCmdInsertDebugUtilsLabelEXT");

	VkDebugUtilsLabelEXT info = {};
	info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
	info.pLabelName = name;
	info.color[0] = color.r;
	info.color[1] = color.g;
	info.color[2] = color.b;
	info.color[3] = color.a;
	vkCmdInsertDebugUtilsLabelEXT_func(commandBuffer, &info);
}

void QueueBeginVkLabel(VkQueue queue, const char * name, const glm::vec4& color)
{
	if (vkQueueBeginDebugUtilsLabelEXT_func == VK_NULL_HANDLE)
		AcquireDebugUtilsFunc< PFN_vkQueueBeginDebugUtilsLabelEXT>(&vkQueueBeginDebugUtilsLabelEXT_func, "vkQueueBeginDebugUtilsLabelEXT");

	VkDebugUtilsLabelEXT info = {};
	info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
	info.pLabelName = name;
	info.color[0] = color.r;
	info.color[1] = color.g;
	info.color[2] = color.b;
	info.color[3] = color.a;
	vkQueueBeginDebugUtilsLabelEXT_func(queue, &info);
}

void QueueEndVkLabel(VkQueue queue)
{
	if (vkQueueEndDebugUtilsLabelEXT_func == VK_NULL_HANDLE)
		AcquireDebugUtilsFunc< PFN_vkQueueEndDebugUtilsLabelEXT>(&vkQueueEndDebugUtilsLabelEXT_func, "vkQueueEndDebugUtilsLabelEXT");

	vkQueueEndDebugUtilsLabelEXT_func(queue);
}

void QueueInsertVkLabel(VkQueue queue, const char * name, const glm::vec4& color)
{
	if (vkQueueInsertDebugUtilsLabelEXT_func == VK_NULL_HANDLE)
		AcquireDebugUtilsFunc< PFN_vkQueueInsertDebugUtilsLabelEXT>(&vkQueueInsertDebugUtilsLabelEXT_func, "vkQueueInsertDebugUtilsLabelEXT");

	VkDebugUtilsLabelEXT info = {};
	info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
	info.pLabelName = name;
	info.color[0] = color.r;
	info.color[1] = color.g;
	info.color[2] = color.b;
	info.color[3] = color.a;
	vkQueueInsertDebugUtilsLabelEXT_func(queue, &info);
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData) {

	std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

	return VK_FALSE;
}

VkResult CreateDebugUtilsMessengerEXT( const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pCallback) {
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr( g_vk.vk_instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(g_vk.vk_instance, pCreateInfo, pAllocator, pCallback);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void DestroyDebugUtilsMessengerEXT( VkDebugUtilsMessengerEXT callback, const VkAllocationCallbacks* pAllocator) {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(g_vk.vk_instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(g_vk.vk_instance, callback, pAllocator);
	}
}

void SetupDebugCallback()
{
	VkDebugUtilsMessengerCreateInfoEXT create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	create_info.pfnUserCallback = debugCallback;
	create_info.pUserData = nullptr; // Optional

	if (CreateDebugUtilsMessengerEXT(&create_info, nullptr, &debug_callback_messenger) != VK_SUCCESS)
		throw std::runtime_error("failed to set up debug callback!");
}

void DestroyDebugCallback()
{
	DestroyDebugUtilsMessengerEXT( debug_callback_messenger, nullptr);
}