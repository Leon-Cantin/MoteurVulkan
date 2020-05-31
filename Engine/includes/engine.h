#pragma once

#include "window_handler.h"
#include "vk_framework.h"
#include "window_handler_vk.h"
#include <unordered_map>
#include <assert.h>
#include <string>

namespace Engine
{
	struct SceneScript
	{
		const char* name = nullptr;
		void( *initCallback )(void);
		void( *updateCallback )(void);
		void( *destroyCallback )(void);
	};

	struct EngineState
	{
		EngineState( void( *initRendererImp )(VkSurfaceKHR), void( *destroyRendererImp )(), const char* name, int window_width, int window_height )
			:_initRendererImp(initRendererImp), _destroyRendererImp(destroyRendererImp), _name(name), _window_width(window_width), _window_height(window_height)
		{
			ZeroMemory( &_currentSceneScript, sizeof( _currentSceneScript ) );
			ZeroMemory( &_nextSceneScript, sizeof( _nextSceneScript ) );
		}
		EngineState()
			:EngineState( nullptr, nullptr, nullptr, 0, 0 )
		{

		}

		void( *_initRendererImp )(VkSurfaceKHR);
		void( *_destroyRendererImp )();

		std::unordered_map< std::string, SceneScript > _scripts_library;

		SceneScript _currentSceneScript;
		SceneScript _nextSceneScript;

		const char * _name;
		int _window_width;
		int _window_height;
	};

	void SetNextScript( const char * scriptName );

	void RegisterSceneScript( EngineState* engineState, const SceneScript& script );

	void Start( const EngineState& engineState, const char* start_script_name );
}