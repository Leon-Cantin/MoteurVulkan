#include "engine.h"

namespace Engine
{
	EngineState _engineState;
	R_HW::DisplaySurface _displaySurface;

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
		vkDeviceWaitIdle( g_gfx.device.device );

		if( engineState->_currentSceneScript.name != nullptr )
			engineState->_currentSceneScript.destroyCallback();

		engineState->_currentSceneScript = engineState->_nextSceneScript;
		MEM::zero( &engineState->_nextSceneScript );
		engineState->_currentSceneScript.initCallback();
	}

	void Start( const EngineState& engineState, const char* start_script_name )
	{
		_engineState = engineState;
		SetNextScript( start_script_name );

		WH::InitializeWindow( _engineState._window_width, _engineState._window_height, _engineState._name );
#ifdef NDEBUG
		const bool useValidationLayer = false;
#else
		const bool useValidationLayer = true;
#endif
		g_gfx.instance = R_HW::CreateInstance( useValidationLayer );
		_displaySurface = WH::VK::create_surface( g_gfx.instance.instance, WH::g_windowState );
		g_gfx.physicalDevice = PickSuitablePhysicalDevice( _displaySurface, g_gfx.instance );
		g_gfx.device = R_HW::create_logical_device( _displaySurface, g_gfx.physicalDevice, useValidationLayer );

		//Init renderer stuff
		_engineState._initRendererImp( &_displaySurface );

		while( !WH::shouldClose() )
		{
			if( NewScriptQueued( _engineState ) )
				SwapScripts( &_engineState );

			//TODO: thread this
			WH::ProcessMessages();
			_engineState._currentSceneScript.updateCallback();
		}

		R_HW::DeviceWaitIdle( g_gfx.device.device );
		_engineState._currentSceneScript.destroyCallback();
		_engineState._destroyRendererImp();

		WH::VK::DestroySurface( &_displaySurface, g_gfx.instance.instance );
		Destroy( &g_gfx.device );
		Destroy( &g_gfx.instance );
		WH::ShutdownWindow();
	}
}