#pragma once

#include "window_handler.h"
#include "window_handler_vk.h"
#include "memory.h"
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

	typedef void InitRendererCallback_t( const DisplaySurface* );
	typedef void DestroyRendererCallback_t();

	struct EngineState
	{
		EngineState( InitRendererCallback_t initRendererCallback, DestroyRendererCallback_t destroyRendererImp, const char* name, int window_width, int window_height )
			:_initRendererImp( initRendererCallback ), _destroyRendererImp(destroyRendererImp), _name(name), _window_width(window_width), _window_height(window_height)
		{
			MEM::zero( &_currentSceneScript );
			MEM::zero( &_nextSceneScript );
		}
		EngineState()
			:EngineState( nullptr, nullptr, nullptr, 0, 0 )
		{

		}

		InitRendererCallback_t* _initRendererImp;
		DestroyRendererCallback_t* _destroyRendererImp;

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