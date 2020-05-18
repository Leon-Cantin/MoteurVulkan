#pragma once

#include "window_handler.h"
#include "vk_framework.h"
#include "window_handler_vk.h"
#include "classic_2d_renderer_imp.h"
#include <unordered_map>

struct SceneScript
{
	const char* name = nullptr;
	void( *initCallback )(void);
	void( *updateCallback )(void);
	void( *destroyCallback )(void);
};

const int VIEWPORT_WIDTH = 224;
const int VIEWPORT_HEIGHT = 384;
const int SCREEN_SCALE = 2;
const int SCREEN_WIDTH = VIEWPORT_WIDTH * SCREEN_SCALE;
const int SCREEN_HEIGHT = VIEWPORT_HEIGHT * SCREEN_SCALE;

namespace Engine
{
	struct cmp_str
	{
		bool operator()( char const *a, char const *b ) const
		{
 			return std::strcmp( a, b ) == 0;
		}
	};

	std::unordered_map<const char*, SceneScript, std::hash<const char *>, cmp_str> _scripts_library;

	SceneScript _currentSceneScript;
	SceneScript _nextSceneScript;

	void SetNextScript( const char * scriptName )
	{
		auto it = _scripts_library.find( scriptName );
		assert( it != _scripts_library.end() );
		_nextSceneScript = it->second;
	}
}

#include "game_classic_2d.h"
SceneScript gameScene { "Game", WildWeasel_Game::Init, WildWeasel_Game::Update, WildWeasel_Game::Destroy };
#include "wild_weasel_main_menu.h"
SceneScript mainMenuScene { "MainMenu", WildWeasel_Menu::Init, WildWeasel_Menu::Update, WildWeasel_Menu::Destroy };

namespace Engine
{
	static bool NewScriptQueued()
	{
		return _nextSceneScript.name != nullptr;
	}

	static void SwapScripts()
	{
		vkDeviceWaitIdle( g_vk.device );
		_currentSceneScript.destroyCallback();
		_currentSceneScript = _nextSceneScript;
		ZeroMemory( &_nextSceneScript, sizeof( _nextSceneScript ) );
		_currentSceneScript.initCallback();
	}

	void Start( SceneScript startScript )
	{
		_currentSceneScript = startScript;
		_scripts_library.insert( { gameScene.name, gameScene } );
		_scripts_library.insert( { mainMenuScene.name, mainMenuScene } );

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

			if( NewScriptQueued() )
				SwapScripts();
		}

		vkDeviceWaitIdle( g_vk.device );

		_currentSceneScript.destroyCallback();

		CleanupRendererImp();

		WH::VK::ShutdownWindow();
		VK::Shutdown();
		WH::ShutdownWindow();
	}
}