#include "vk_framework.h"
#include "vk_debug.h"
#include "window_handler.h"

#include <iostream>
#include <algorithm>
#include <set>

#ifdef NDEBUG
const bool enableValidationLayers = false;
const bool enableVkObjectMarking = true;
#else
const bool enableValidationLayers = true;
const bool enableVkObjectMarking = true;
#endif

VkSurfaceKHR g_windowSurface;

bool framebuffer_resized = false;

static void framebuffer_resize_callback(GLFWwindow* window, int width, int height)
{
	framebuffer_resized = true;
}

const std::vector<const char*> validationLayers = {
	"VK_LAYER_LUNARG_standard_validation"
};

const std::vector<const char*> required_device_extensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

static std::vector<const char*> get_required_extensions()
{
	//glfwExtensions
	uint32_t glfwExtensionCount = 0;
	const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	//Other extensions
	std::vector<const char*> extensions(glfw_extensions, glfw_extensions + glfwExtensionCount);

	if (enableValidationLayers || enableVkObjectMarking )
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

	std::cout << "required extensions :" << std::endl;
	for (uint32_t i = 0; i < extensions.size(); ++i)
		std::cout << "\t" << extensions[i] << std::endl;

	return extensions;
}

static uint32_t detect_available_extensions(VkExtensionProperties* extensions)
{
	uint32_t extension_count = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
	vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extensions);

	std::cout << "Available extension: " << std::endl;
	for (uint32_t i = 0; i < extension_count; ++i)
		std::cout << "\t" << extensions[i].extensionName << std::endl;

	return extension_count;
}

static bool check_extensions(const char** required_extensions, uint32_t required_extensions_count)
{
	uint32_t extension_count = 0;

	//Get available extensions
	vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
	std::vector<VkExtensionProperties> detected_extensions(extension_count);
	detect_available_extensions(detected_extensions.data());

	//Check if all required are available
	std::vector<bool> missing(required_extensions_count);
	bool any_missing = false;
	for (uint32_t i = 0; i < required_extensions_count; ++i)
	{
		bool found = false;
		for (VkExtensionProperties& extension : detected_extensions)
		{
			if (strcmp(required_extensions[i], extension.extensionName) == 0)
			{
				found = true;
				break;
			}
		}
		if (!found)
		{
			missing[i] = true;
			any_missing = true;
		}
	}

	//Print output
	if (any_missing)
	{
		std::cout << "Error Missing extensions:" << std::endl;
		for (uint32_t i = 0; i < required_extensions_count; ++i)
			if (missing[i])
				std::cout << "\t" << required_extensions[i] << std::endl;
	}
	else
		std::cout << "Extensions check OK" << std::endl;

	return !any_missing;
}

static uint32_t detect_supported_validation_layers(VkLayerProperties * available_layers) {
	uint32_t layerCount = 0;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	vkEnumerateInstanceLayerProperties(&layerCount, available_layers);

	std::cout << "Available validation layers: " << std::endl;
	for (uint32_t i = 0; i < layerCount; ++i)
		std::cout << "\t" << available_layers[i].layerName << std::endl;

	return layerCount;
}

static bool check_validation_layers(const char* const* required_layers, uint32_t required_layers_count)
{
	std::cout << "Required validation layers :" << std::endl;
	for (uint32_t i = 0; i < required_layers_count; ++i)
		std::cout << "\t" << required_layers[i] << std::endl;

	uint32_t layers_count = 0;

	//Get available extensions
	vkEnumerateInstanceLayerProperties(&layers_count, nullptr);
	std::vector<VkLayerProperties> detected_layers(layers_count);
	detect_supported_validation_layers(detected_layers.data());

	//Check if all required are available
	std::vector<bool> missing(layers_count);
	bool any_missing = false;
	for (uint32_t i = 0; i < required_layers_count; ++i)
	{
		bool found = false;
		for (VkLayerProperties& layer : detected_layers)
		{
			if (strcmp(required_layers[i], layer.layerName) == 0)
			{
				found = true;
				break;
			}
		}
		if (!found)
		{
			missing[i] = true;
			any_missing = true;
		}
	}

	//Print output
	if (any_missing)
	{
		std::cout << "Error missing validation layers:" << std::endl;
		for (uint32_t i = 0; i < required_layers_count; ++i)
			if (missing[i])
				std::cout << "\t" << required_layers[i] << std::endl;
	}
	else
		std::cout << "Validation layers check OK" << std::endl;

	return !any_missing;
}

static void create_vk_instance()
{
	VkApplicationInfo app_info = {};
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pApplicationName = "Hello triangle";
	app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.pEngineName = "No Engine";
	app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.apiVersion = VK_API_VERSION_1_1;

	VkInstanceCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	create_info.pApplicationInfo = &app_info;

	//Check extensions
	std::vector<const char*> required_extensions = get_required_extensions();
	check_extensions(required_extensions.data(), static_cast<uint32_t>(required_extensions.size()));

	create_info.enabledExtensionCount = static_cast<uint32_t>(required_extensions.size());
	create_info.ppEnabledExtensionNames = required_extensions.data();

	//Check validation layers
	if (enableValidationLayers)
	{
		uint32_t validation_layers_count = static_cast<uint32_t>(validationLayers.size());
		check_validation_layers(validationLayers.data(), validation_layers_count);

		create_info.ppEnabledLayerNames = validationLayers.data();
		create_info.enabledLayerCount = validation_layers_count;
	}
	else
	{
		create_info.enabledLayerCount = 0;
	}

	if (vkCreateInstance(&create_info, nullptr, &g_vk.vk_instance) != VK_SUCCESS)
		throw std::runtime_error("failed to create instance");
}

static bool check_device_extension_support(VkPhysicalDevice device) {
	uint32_t extension_count;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);

	std::vector<VkExtensionProperties> available_extensions(extension_count);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, available_extensions.data());

	std::set<std::string> requiredExtensions(required_device_extensions.begin(), required_device_extensions.end());

	for (const auto& extension : available_extensions) {
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}

static bool is_device_suitable(const VkPhysicalDevice device)
{
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);

	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

	bool suitable = deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
	suitable |= find_queue_families(device).is_complete();
	suitable |= check_device_extension_support(device);
	suitable |= deviceFeatures.samplerAnisotropy == VK_TRUE;
	suitable |= deviceFeatures.depthClamp == VK_TRUE;
	suitable |= deviceFeatures.shaderSampledImageArrayDynamicIndexing == VK_TRUE;

	if (suitable) {
		SwapChainSupportDetails swapchain_details = query_swap_chain_support(device, g_windowSurface);
		suitable |= !swapchain_details.formats.empty() && !swapchain_details.present_modes.empty();
	}

	return suitable;
}

static void pick_physical_device()
{
	uint32_t device_count = 0;
	vkEnumeratePhysicalDevices(g_vk.vk_instance, &device_count, nullptr);
	if (device_count == 0) {
		throw std::runtime_error("failed to find GPUs with Vulkan support!");
	}

	std::vector<VkPhysicalDevice> devices(device_count);
	vkEnumeratePhysicalDevices(g_vk.vk_instance, &device_count, devices.data());

	for (const auto& device : devices) {
		if (is_device_suitable(device)) {
			g_vk.physicalDevice = device;
			break;
		}
	}

	if (g_vk.physicalDevice == VK_NULL_HANDLE) {
		throw std::runtime_error("failed to find a suitable GPU!");
	}

}

static void create_logical_device()
{
	//Queues
	QueueFamilyIndices indices = find_queue_families(g_vk.physicalDevice);

	std::set<uint32_t> unique_queue_families = { indices.graphics_family.value(), indices.present_family.value(), indices.compute_family.value(), indices.transfer_family.value() };
	std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
	queue_create_infos.reserve(unique_queue_families.size());

	for (uint32_t queue_family_index : unique_queue_families) {
		VkDeviceQueueCreateInfo queue_create_info = {};
		queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_create_info.queueFamilyIndex = queue_family_index;
		queue_create_info.queueCount = 1;
		float queue_priority = 1.0f;
		queue_create_info.pQueuePriorities = &queue_priority;
		queue_create_infos.push_back(queue_create_info);
	}

	//Features
	VkPhysicalDeviceFeatures device_features = {};
	device_features.samplerAnisotropy = VK_TRUE;
	device_features.depthClamp = VK_TRUE;
	device_features.shaderSampledImageArrayDynamicIndexing = VK_TRUE;

	VkDeviceCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	create_info.pQueueCreateInfos = queue_create_infos.data();
	create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
	create_info.pEnabledFeatures = &device_features;

	//Exensions
	create_info.enabledExtensionCount = static_cast<uint32_t>(required_device_extensions.size());
	create_info.ppEnabledExtensionNames = required_device_extensions.data();

	if (enableValidationLayers) {
		create_info.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		create_info.ppEnabledLayerNames = validationLayers.data();
	}
	else {
		create_info.enabledLayerCount = 0;
	}

	if (vkCreateDevice(g_vk.physicalDevice, &create_info, nullptr, &g_vk.device) != VK_SUCCESS)
		throw std::runtime_error("failed to create logical device!");

	vkGetDeviceQueue(g_vk.device, indices.graphics_family.value(), 0, &g_vk.graphics_queue);
	vkGetDeviceQueue(g_vk.device, indices.present_family.value(), 0, &g_vk.present_queue);
	vkGetDeviceQueue(g_vk.device, indices.compute_family.value(), 0, &g_vk.compute_queue);
	vkGetDeviceQueue(g_vk.device, indices.transfer_family.value(), 0, &g_vk.transfer_queue);

	MarkVkObject((uint64_t)(g_vk.graphics_queue), VK_OBJECT_TYPE_QUEUE, "graphics_queue");
	MarkVkObject((uint64_t)(g_vk.present_queue), VK_OBJECT_TYPE_QUEUE, "present_queue");
	MarkVkObject((uint64_t)(g_vk.compute_queue), VK_OBJECT_TYPE_QUEUE, "compute_queue");
	MarkVkObject((uint64_t)(g_vk.transfer_queue), VK_OBJECT_TYPE_QUEUE, "transfer_queue");
}

SwapChainSupportDetails query_swap_chain_support(VkPhysicalDevice device, VkSurfaceKHR surface) {
	SwapChainSupportDetails details;

	//Capabilities
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

	//Formats
	uint32_t format_count;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, nullptr);

	if (format_count != 0) {
		details.formats.resize(format_count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, details.formats.data());
	}

	//Present modes
	uint32_t present_mode_count;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, nullptr);

	if (present_mode_count != 0) {
		details.present_modes.resize(present_mode_count);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, details.present_modes.data());
	}

	return details;
}

QueueFamilyIndices find_queue_families(const VkPhysicalDevice device) {
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	for (uint32_t i = 0; i < queueFamilyCount; ++i)
	{
		const VkQueueFamilyProperties& queueProperties = queueFamilies[i];
		if (queueProperties.queueCount > 0)
		{
			if (queueProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT
				&& (queueProperties.timestampValidBits > 0)
				&& (!indices.graphics_family.has_value()))
				indices.graphics_family = i;

			if (!(queueProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT)
				&& (queueProperties.queueFlags & VK_QUEUE_COMPUTE_BIT)
				&& (!indices.compute_family.has_value()))
				indices.compute_family = i;

			if (queueProperties.queueFlags & VK_QUEUE_TRANSFER_BIT
				&& !(queueProperties.queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT))
				&& (!indices.transfer_family.has_value()))
				indices.transfer_family = i;

			VkBool32 present_support = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, g_windowSurface, &present_support);
			if (present_support && !indices.present_family.has_value())
				indices.present_family = i;

			if (indices.is_complete())
				break;
		}
	}

	return indices;
}

void InitFramework(int windowWidth, int windowHeight, const char * windowName)
{
	WH::init_window(windowWidth, windowHeight, windowName);
	WH::add_framebuffer_resize_callback(framebuffer_resize_callback);
	create_vk_instance();
	if (enableValidationLayers)
		SetupDebugCallback();
	WH::create_surface(&g_windowSurface);
	pick_physical_device();
	create_logical_device();
}

void ShutdownFramework()
{
	vkDestroyDevice(g_vk.device, nullptr);

	if (enableValidationLayers)
		DestroyDebugCallback();

	vkDestroySurfaceKHR(g_vk.vk_instance, g_windowSurface, nullptr);
	vkDestroyInstance(g_vk.vk_instance, nullptr);

	WH::terminate();
}