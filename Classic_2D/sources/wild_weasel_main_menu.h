#pragma once

#include "input.h"
#include "console_command.h"
#include "classic_2d_renderer_imp.h"
#include "gfx_heaps_batched_allocator.h"
#include "asset_library.h"
#include "classic_2d_gfx_asset.h"
#include "window_handler.h"
#include "engine.h"

namespace WildWeasel_Menu
{
	uint32_t current_frame = 0;
	size_t frameDeltaTime = 0;

	GfxAsset shipRenderable;
	GfxAsset backgroundRenderable;
	GfxHeap gfx_heap;
	BindlessTexturesState bindlessTexturesState;
	SceneInstance cameraSceneInstance;
	SceneInstance shipSceneInstance;
	SceneInstance backgroundSceneInstance;

	constexpr float shipSize = 21.0f;

	void Update() 
	{
		size_t currentTime = WH::GetTime();
		static size_t lastTime = currentTime;

		frameDeltaTime = static_cast< size_t >(currentTime - lastTime);
		lastTime = currentTime;

		//Input
		IH::DoCommands();

		//Update objects
		//TickUpdate( frameDeltaTime );

		std::vector<GfxAssetInstance> drawList;
		drawList.push_back( { &shipRenderable, shipSceneInstance, false } );
		drawList.push_back( { &backgroundRenderable, backgroundSceneInstance, false } );

		std::vector<TextZone> textZones;
		DrawFrame( current_frame, &cameraSceneInstance, drawList, textZones );

		current_frame = (++current_frame) % SIMULTANEOUS_FRAMES;
	}

	const float movementSpeedPerSecond = 100.0f;
	float GetMovement( size_t frameDeltaTime )
	{
		return movementSpeedPerSecond * (frameDeltaTime / 1000.0f);
	}

	void ForwardCallback()
	{
		shipSceneInstance.location.y += GetMovement( frameDeltaTime );
	}

	void BackwardCallback()
	{
		shipSceneInstance.location.y -= GetMovement( frameDeltaTime );
	}

	void FireCallback()
	{
		Engine::SetNextScript( "Game" );
	}

	void Init()
	{
		//Input callbacks
		IH::InitInputs();
		IH::RegisterAction( "Forward", IH::W );
		IH::BindAction( "Forward", IH::Pressed, &ForwardCallback );
		IH::RegisterAction( "backward", IH::S );
		IH::BindAction( "backward", IH::Pressed, &BackwardCallback );
		IH::RegisterAction( "fire", IH::SPACE );
		IH::BindAction( "fire", IH::Pressed, &FireCallback );

		IH::RegisterAction( "console", IH::TILD );
		IH::BindAction( "console", IH::Pressed, &ConCom::OpenConsole );

		//Console commands callback (need IH)
		ConCom::Init();

		//Objects update callbacks
		//RegisterTickFunction( &TickObjectCallback );

		//LoadAssets
		uint32_t memoryTypeMask = 15;/*TODO this works for now for textures, comes from asking VK for a textures*/
		gfx_heap = create_gfx_heap( 16 * 1024 * 1024, GFX_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
		GfxHeaps_BatchedAllocator gfx_mem_allocator( &gfx_heap );
		gfx_mem_allocator.Prepare();

		GfxImage* shipTexture = AL::LoadTexture( "shipTexture", "assets/F14.png", &gfx_mem_allocator );
		GfxImage* mainMenuBackgroundTexture = AL::LoadTexture( "mainMenuBackground", "assets/main_menu_background.png", &gfx_mem_allocator );

		GfxModel* quadModel = AL::CreateQuad( "Quad", quadSize, &gfx_mem_allocator );
		GfxModel* backgroundQuadModel = AL::CreateQuad( "BackgroundQuad", VIEWPORT_WIDTH, VIEWPORT_HEIGHT, &gfx_mem_allocator );

		uint32_t shipTextureIndex = RegisterBindlessTexture( &bindlessTexturesState, shipTexture, eSamplers::Point );
		uint32_t mainMenuBackgroundTextureIndex = RegisterBindlessTexture( &bindlessTexturesState, mainMenuBackgroundTexture, eSamplers::Point );

		gfx_mem_allocator.Commit();

		shipRenderable = CreateGfxAsset( quadModel, shipTextureIndex );
		backgroundRenderable = CreateGfxAsset( backgroundQuadModel, mainMenuBackgroundTextureIndex );

		CompileScene( &bindlessTexturesState );

		shipSceneInstance = { glm::vec3( 0.0f, 0.0f, 2.0f ), defaultRotation, shipSize };
		backgroundSceneInstance = { glm::vec3( 0.0f, 0.0f, 3.0f ), defaultRotation, 1.0f };
		cameraSceneInstance = { glm::vec3( 0.0f, 0.0f, -2.0f ), defaultRotation, 1.0f };
	}

	void Destroy()
	{
		//TODO: "Decompile scene"
		IH::CleanupInputs();
		ConCom::Cleanup();

		AL::Cleanup();

		destroy( &gfx_heap );
	}
}
