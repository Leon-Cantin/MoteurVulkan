#pragma once

#include "window_handler.h"
#include "vk_framework.h"
#include "window_handler_vk.h"
#include "classic_2d_renderer_imp.h"

struct SceneScript
{
	void( *initCallback )(void);
	void( *updateCallback )(void);
	void( *destroyCallback )(void);
};

const int VIEWPORT_WIDTH = 224;
const int VIEWPORT_HEIGHT = 384;
const int SCREEN_SCALE = 2;
const int SCREEN_WIDTH = VIEWPORT_WIDTH * SCREEN_SCALE;
const int SCREEN_HEIGHT = VIEWPORT_HEIGHT * SCREEN_SCALE;

#include "game_classic_2d.h"
SceneScript mainScene { WildWeasel_Game::Init, WildWeasel_Game::Update, WildWeasel_Game::Destroy };

namespace Engine
{
	SceneScript _currentSceneScript;
	SceneScript _nextSceneScript;

	void Start( SceneScript startScript )
	{
		_currentSceneScript = startScript;

		WH::InitializeWindow( SCREEN_WIDTH, SCREEN_HEIGHT, "Wild Weasel: Vietnam" );
		VK::Initialize();
		WH::VK::InitializeWindow();
		VK::PickSuitablePhysicalDevice( WH::VK::_windowSurface );

		//Init renderer stuff
		InitRendererImp( WH::VK::_windowSurface );

		_currentSceneScript.initCallback();
		while( !WH::shouldClose() )
		{
			//TODO: thread this
			WH::ProcessMessages();
			_currentSceneScript.updateCallback();

			//TODO: check if new script. Destroy old, recreate new;
		}

		vkDeviceWaitIdle( g_vk.device );

		_currentSceneScript.destroyCallback();

		CleanupRendererImp();

		WH::VK::ShutdownWindow();
		VK::Shutdown();
		WH::ShutdownWindow();
	}
}