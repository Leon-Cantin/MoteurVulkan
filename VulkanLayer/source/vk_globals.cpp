#include "vk_globals.h"

Vk_Globals g_vk;

void DeviceWaitIdle( GfxDevice device )
{
	vkDeviceWaitIdle( g_vk.device.device );
}