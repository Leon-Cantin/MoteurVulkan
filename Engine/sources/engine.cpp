#include "engine.h"

namespace Engine
{
	EngineState _engineState;

	void SetNextScript( const char * scriptName )
	{
		auto it = _engineState._scripts_library.find( scriptName );
		assert( it != _engineState._scripts_library.end() );
		_engineState._nextSceneScript = it->second;
	}

	void RegisterSceneScript( EngineState* engineState, const SceneScript& script )
	{
		engineState->_scripts_library.insert( { script.name, script } );
	}

	static bool NewScriptQueued( const EngineState& engineState )
	{
		return engineState._nextSceneScript.name != nullptr;
	}

	static void SwapScripts( EngineState* engineState )
	{
		vkDeviceWaitIdle( g_vk.device );

		if( engineState->_currentSceneScript.name != nullptr )
			engineState->_currentSceneScript.destroyCallback();

		engineState->_currentSceneScript = engineState->_nextSceneScript;
		ZeroMemory( &engineState->_nextSceneScript, sizeof( engineState->_nextSceneScript ) );
		engineState->_currentSceneScript.initCallback();
	}

	void Start( const EngineState& engineState, const char* start_script_name )
	{
		_engineState = engineState;
		SetNextScript( start_script_name );

		WH::InitializeWindow( _engineState._window_width, _engineState._window_height, _engineState._name );
		VK::Initialize();
		WH::VK::InitializeWindow();
		VK::PickSuitablePhysicalDevice( WH::VK::_windowSurface );

		//Init renderer stuff
		_engineState._initRendererImp( WH::VK::_windowSurface );

		while( !WH::shouldClose() )
		{
			if( NewScriptQueued( _engineState ) )
				SwapScripts( &_engineState );

			//TODO: thread this
			WH::ProcessMessages();
			_engineState._currentSceneScript.updateCallback();
		}

		vkDeviceWaitIdle( g_vk.device );
		_engineState._currentSceneScript.destroyCallback();
		_engineState._destroyRendererImp();

		WH::VK::ShutdownWindow();
		VK::Shutdown();
		WH::ShutdownWindow();
	}
}