#pragma once

#include "vk_globals.h"

#include <vector>
#include <optional>

namespace VK
{
	GpuInstance CreateInstance( bool useValidationLayer );
	PhysicalDevice PickSuitablePhysicalDevice( DisplaySurface swapchainSurface, GpuInstance& instance );
	Device create_logical_device( DisplaySurface swapchainSurface, PhysicalDevice physicalDevice, bool enableValidationLayers );

	void Destroy( Device* device );
	void Destroy( GpuInstance* gpuInstance );
}