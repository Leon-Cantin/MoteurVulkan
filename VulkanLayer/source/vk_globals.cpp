#include "vk_globals.h"

Gfx_Globals g_gfx;

void DeviceWaitIdle( GfxDevice device )
{
	vkDeviceWaitIdle( g_gfx.device.device );
}