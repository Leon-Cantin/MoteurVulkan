#pragma once

#include "classic_2d_renderer_imp.h"
#include "vk_framework.h"
#include "console_command.h"
#include "input.h"
#include "tick_system.h"
#include "asset_library.h"
#include "window_handler_vk.h"

#include "model_asset.h"

#include <glm/glm.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <chrono>
#include <iostream>
#include <vector>
#include <algorithm>
#include <assert.h>

namespace Scene2DGame
{
	uint32_t current_frame = 0;

	const int WIDTH = 800;
	const int HEIGHT = 1000;

	SceneInstance enemyShipSceneInstance;
	SceneInstance shipSceneInstance;
	GfxAsset shipRenderable;
	SceneInstance bakcgroundSceneInstance;
	GfxAsset backgroundAsset;

	SceneInstance cameraSceneInstance;

	BindlessTexturesState bindlessTexturesState;

	size_t frameDeltaTime = 0;

	struct BulletInstance
	{
		float lifeTime;
		float maxLifetime;
		SceneInstance sceneInstance;
	};

	GfxAsset bulletRenderable;
	std::vector<BulletInstance> bulletInstances;

	bool UpdateBullet( size_t deltaTime, BulletInstance* instance )
	{
		instance->lifeTime += deltaTime;
		if( instance->lifeTime > instance->maxLifetime )
			return false;
		instance->sceneInstance.location.y += 50.0f * (frameDeltaTime / 1000.0f);
		//TODO: check collision
		float dx = abs( enemyShipSceneInstance.location.x - instance->sceneInstance.location.x );
		float dy = abs( enemyShipSceneInstance.location.y - instance->sceneInstance.location.y );
		if( sqrt( dx*dx + dy * dy ) < 1.0f )
			return false;
		return true;
	}

	void UpdateBullets( size_t deltaTime )
	{
		auto itEraseFirst = bulletInstances.end();
		auto itEraseLast = bulletInstances.end();
		for( auto i = bulletInstances.begin(); i != bulletInstances.end(); ++i )
		{
			bool isAlive = UpdateBullet( deltaTime, i._Ptr );
			if( !isAlive )
			{
				if( itEraseFirst == bulletInstances.end() )
					itEraseFirst = i;
				itEraseLast = i+1;
			}
		}
		if( itEraseFirst != bulletInstances.end() )
		{
			bulletInstances.erase( itEraseFirst, itEraseLast );
		}
	}

	void createBullet()
	{
		BulletInstance bulletInstance = { 0.0f, 2000.0f, { shipSceneInstance.location, shipSceneInstance.orientation, 1.0f } };
		bulletInstances.push_back( bulletInstance );
	}

	const float movementSpeed = 10.0f;
	float GetMovement( size_t frameDeltaTime )
	{
		return movementSpeed * ( frameDeltaTime / 1000.0f );
	}

	void ForwardCallback()
	{
		shipSceneInstance.location.y += GetMovement( frameDeltaTime );
	}

	void BackwardCallback()
	{
		shipSceneInstance.location.y -= GetMovement( frameDeltaTime );
	}

	void MoveRightCallback()
	{
		shipSceneInstance.location.x += GetMovement( frameDeltaTime );
	}

	void MoveLeftCallback()
	{
		shipSceneInstance.location.x -= GetMovement( frameDeltaTime );
	}

	void FireCallback()
	{
		size_t currentTime = WH::GetTime();
		static size_t lastTime = 0;

		if( currentTime - lastTime > 50 || lastTime == 0 )
		{
			createBullet();
			lastTime = currentTime;
		}
	}

	void mainLoop() {
		while (!WH::shouldClose())
		{
			//TODO: thread this
			WH::ProcessMessages();
			size_t currentTime = WH::GetTime();
			static size_t lastTime = currentTime;
			
			frameDeltaTime = static_cast<size_t>(currentTime - lastTime);
			lastTime = currentTime;

			//Input
			IH::DoCommands();

			//Update objects
			TickUpdate(frameDeltaTime);

			UpdateBullets( frameDeltaTime );

			std::vector<GfxAssetInstance> drawList = { { &backgroundAsset, bakcgroundSceneInstance }, { &shipRenderable, shipSceneInstance }, { &shipRenderable, enemyShipSceneInstance } };
			for( BulletInstance& bi : bulletInstances )
				drawList.push_back( { &bulletRenderable, bi.sceneInstance } );

			DrawFrame( current_frame, &cameraSceneInstance, drawList);

			current_frame = (++current_frame) % SIMULTANEOUS_FRAMES;
		}

		vkDeviceWaitIdle(g_vk.device);
	}

	GfxAsset CreateGfxAsset( const GfxModel* modelAsset, uint32_t albedoIndex )
	{
		GfxAsset asset = { modelAsset, { albedoIndex } };
		return asset;
	}

	void Init()
	{
		WH::InitializeWindow( WIDTH, HEIGHT, "2D game" );
		VK::Initialize();
		WH::VK::InitializeWindow();
		VK::PickSuitablePhysicalDevice( WH::VK::_windowSurface );

		//Input callbacks
		IH::InitInputs();
		IH::RegisterAction( "console", IH::Pressed, &ConCom::OpenConsole );
		IH::BindInputToAction( "console", IH::TILD );
		IH::RegisterAction( "forward", IH::Pressed, &ForwardCallback );
		IH::BindInputToAction( "forward", IH::W );
		IH::RegisterAction( "backward", IH::Pressed, &BackwardCallback );
		IH::BindInputToAction( "backward", IH::S );
		IH::RegisterAction( "left", IH::Pressed, &MoveLeftCallback );
		IH::BindInputToAction( "left", IH::A );
		IH::RegisterAction( "right", IH::Pressed, &MoveRightCallback );
		IH::BindInputToAction( "right", IH::D );
		IH::RegisterAction( "fire", IH::Pressed, &FireCallback );
		IH::BindInputToAction( "fire", IH::E );

		//Console commands callback (need IH)
		ConCom::Init();

		//Objects update callbacks
		//RegisterTickFunction( &TickObjectCallback );

		//Init renderer stuff
		InitRendererImp( WH::VK::_windowSurface );

		//LoadAssets
		GfxImage* shipTexture = AL::LoadTexture( "shipTexture", "assets/ship.png" );
		GfxImage* bulletTexture = AL::LoadTexture( "bullet_texture", "assets/bullet.png" );
		GfxImage* backgroundTexture = AL::CreateSolidColorTexture( "background_texture", { 0.0f, 0.1f, 0.8f, 1.0f } );

		GfxModel* quadModel = AL::CreateQuad( "Quad", 1.0f );

		uint32_t shipTextureIndex = RegisterBindlessTexture( &bindlessTexturesState, shipTexture, eSamplers::Point );
		uint32_t bulletTextureIndex = RegisterBindlessTexture( &bindlessTexturesState, bulletTexture, eSamplers::Point );
		uint32_t backgroundTextureIndex = RegisterBindlessTexture( &bindlessTexturesState, backgroundTexture, eSamplers::Point );

		shipRenderable = CreateGfxAsset( quadModel, shipTextureIndex );
		bulletRenderable = CreateGfxAsset( quadModel, bulletTextureIndex );
		backgroundAsset = CreateGfxAsset( quadModel, backgroundTextureIndex );

		CompileScene( &bindlessTexturesState );

		enemyShipSceneInstance = { glm::vec3( 0.0f, 5.0f, 2.0f ), glm::angleAxis( glm::radians( 0.0f ), glm::vec3{0.0f, 1.0f, 0.0f} ), -2.0f };
		shipSceneInstance = { glm::vec3( 0.0f, 0.0f, 2.0f ), glm::angleAxis( glm::radians( 0.0f ), glm::vec3{0.0f, 1.0f, 0.0f} ), 2.0f };
		cameraSceneInstance = { glm::vec3( 0.0f, 0.0f, -2.0f ), glm::angleAxis( glm::radians( 0.0f ), glm::vec3{0.0f, 1.0f, 0.0f} ), 1.0f };
		bakcgroundSceneInstance = { glm::vec3( 0.0f, 0.0f, 8.0f ), glm::angleAxis( glm::radians( 0.0f ), glm::vec3{0.0f, 1.0f, 0.0f} ), 500.0f };
	}

	void cleanup() 
	{
		CleanupRendererImp();

		AL::Cleanup();
	}

	void run() 
	{
		Init();
		mainLoop();
		cleanup();
		WH::VK::ShutdownWindow();
		VK::Shutdown();
		WH::ShutdownWindow();
	}
}