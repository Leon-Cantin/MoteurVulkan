#pragma once

#include "classic_2d_renderer_imp.h"
#include "vk_framework.h"
#include "console_command.h"
#include "input.h"
#include "tick_system.h"
#include "asset_library.h"
#include "window_handler_vk.h"
#include "gfx_heaps.h"
#include "gfx_heaps_batched_allocator.h"
#include "classic_2d_gfx_asset.h"
#include "terrain.h"

#include <glm/glm.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <chrono>
#include <iostream>
#include <vector>
#include <algorithm>
#include <assert.h>

namespace WildWeasel_Game
{
	uint32_t current_frame = 0;

	struct EnemyShipInstance
	{
		float currentHealth;
		float maxHealth;
		SceneInstance sceneInstance;
	};

	struct BulletInstance
	{
		float lifeTime;
		float maxLifetime;
		SceneInstance sceneInstance;
	};

	std::vector<BulletInstance> bulletInstances;
	std::vector<EnemyShipInstance> enemyShipSceneInstances;
	BackgroundInstance backgroundInstance;
	std::vector<SceneInstance> cloudInstances;
	SceneInstance shipSceneInstance;
	SceneInstance cameraSceneInstance;

	GfxAsset shipRenderable;
	GfxAsset bulletRenderable;
	GfxAsset treeAsset;
	GfxAsset riverBankAsset;
	GfxAsset riverAsset;
	GfxAsset cloudAsset;

	BindlessTexturesState bindlessTexturesState;

	size_t frameDeltaTime = 0;
	uint32_t _score = 0;

	GfxHeap gfx_heap;

	bool UpdateInstance( size_t deltaTime, EnemyShipInstance* enemyShipInstance )
	{
		constexpr float movementSpeedPerSecond = 100.0f;
		constexpr float yKill = -200.0f;
		if( enemyShipInstance->currentHealth <= 0 )
		{
			++_score;
			return false;
		}
		enemyShipInstance->sceneInstance.location.y -= movementSpeedPerSecond * (deltaTime / 1000.0f);
		if( enemyShipInstance->sceneInstance.location.y < yKill )
			return false;
		return true;
	}

	constexpr float shipSize = 21.0f;

	void CreateEnemyShip()
	{
		const float x = ((float)std::rand()/ RAND_MAX) * 150.0f -75.0f;
		constexpr float y = 200.0f;
		EnemyShipInstance enemyShip = { 5.0f, 5.0f, { glm::vec3( x, y, 2.0f ), defaultRotation, -shipSize } };
		enemyShipSceneInstances.push_back( enemyShip );
	}

	void CreateCloud()
	{
		//TODO: reuse variables
		const float x = (( float )std::rand() / RAND_MAX) * 150.0f - 75.0f;
		const float size = (( float )std::rand() / RAND_MAX) * 10.0f + 10.0f;
		constexpr float y = 200.0f;
		SceneInstance instance = { glm::vec3( x, y, 7.0f ), defaultRotation, size };
		cloudInstances.push_back( instance );
	}

	void createBullet()
	{
		constexpr float bulletSize = 7.0f;
		static bool left = false;
		left ^= true;
		const float bullet_offset = 2.0f;
		const float xoffset = left ? -bullet_offset : bullet_offset;
		const glm::vec3 offset( xoffset, 0.0f, 0.0f );
		BulletInstance bulletInstance = { 0.0f, 2000.0f, { shipSceneInstance.location + offset, shipSceneInstance.orientation, bulletSize } };
		bulletInstances.push_back( bulletInstance );
	}

	bool UpdateInstance( size_t deltaTime, BulletInstance* instance )
	{
		constexpr float bulletSpeedPerSecond = 150.0f;
		constexpr float collisionSphereRayLenght = 10.0f;
		instance->lifeTime += deltaTime;
		if( instance->lifeTime > instance->maxLifetime )
			return false;
		instance->sceneInstance.location.y += bulletSpeedPerSecond * (deltaTime / 1000.0f);
		for( auto& enemyShipSceneInstance : enemyShipSceneInstances )
		{
			float dx = abs( enemyShipSceneInstance.sceneInstance.location.x - instance->sceneInstance.location.x );
			float dy = abs( enemyShipSceneInstance.sceneInstance.location.y - instance->sceneInstance.location.y );
			if( sqrt( dx*dx + dy * dy ) < collisionSphereRayLenght )
			{
				enemyShipSceneInstance.currentHealth -= 1.0f;
				return false;
			}
		}
		return true;
	}

	//For clouds
	bool UpdateInstance( size_t deltaTime, SceneInstance* instance )
	{
		constexpr float movementSpeedPerSecond = 100.0f;
		constexpr float yKill = -200.0f;
		instance->location.y -= movementSpeedPerSecond * (deltaTime / 1000.0f);
		if( instance->location.y < yKill )
			return false;
		return true;
	}

	//Keeps the order when deletinga
	template< typename T >
	void UpdateInstanceList( std::vector<T>& instances, size_t deltaTime )
	{
		uint32_t deadRangesCounts = 0;
		uint32_t aliveCount = 0;
		std::pair< std::vector<T>::iterator, std::vector<T>::iterator > deadRanges [16];
		bool isDeleting = false;

		for( auto i = instances.begin(); i != instances.end(); ++i )
		{
			const bool isAlive = UpdateInstance( deltaTime, i._Ptr );
			if( !isAlive )
			{
				if( !isDeleting )
				{
					isDeleting = true;
					deadRanges[deadRangesCounts].first = i;
				}
			}
			else
			{
				aliveCount++;
				if( isDeleting )
				{
					isDeleting = false;
					deadRanges[deadRangesCounts].second = i;
					deadRangesCounts++;
				}
			}
		}
		if( isDeleting )
		{
			isDeleting = false;
			deadRanges[deadRangesCounts].second = instances.end();
			deadRangesCounts++;
		}

		for( uint32_t i = 0; i < deadRangesCounts; ++i )
		{
			T* freeRangeStart = deadRanges[i].first._Ptr;
			T* copyRangeStart = deadRanges[i].second._Ptr;
			T* copyRangeEnd = i+1< deadRangesCounts ? deadRanges[i + 1].first._Ptr : instances.end()._Ptr;

			size_t sizeToCopy = (copyRangeEnd - copyRangeStart) * sizeof(T);

			memcpy( freeRangeStart, copyRangeStart, sizeToCopy );
		}

		instances.resize( aliveCount );
	}

	const float movementSpeedPerSecond = 100.0f;
	float GetMovement( size_t frameDeltaTime )
	{
		return movementSpeedPerSecond * ( frameDeltaTime / 1000.0f );
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

	std::vector<TextZone> UpdateText()
	{
		std::vector<TextZone> textZones;
		char textBuffer[16];
		int charCount = sprintf_s( textBuffer, 16, "Score: %i", _score );
		textZones.push_back( { -1.0f, -1.0f, std::string( textBuffer ) } );

		if( ConCom::isOpen() )
			textZones.push_back( { -1.0f, 0.0f, ConCom::GetViewableString() } );
		
		return textZones;
	}

	void Update() {

		size_t currentTime = WH::GetTime();
		static size_t lastTime = currentTime;
			
		frameDeltaTime = static_cast<size_t>(currentTime - lastTime);
		lastTime = currentTime;

		//Input
		IH::DoCommands();

		static size_t lastSpawn = 0;
		if( lastSpawn == 0 || currentTime - lastSpawn > 1000 )
		{
			CreateEnemyShip();
			CreateCloud();
			lastSpawn = currentTime;
		}

		UpdateBackgroundScrolling( &backgroundInstance, frameDeltaTime );

		//Update objects
		//TickUpdate( frameDeltaTime );

		UpdateInstanceList( bulletInstances, frameDeltaTime );
		UpdateInstanceList( enemyShipSceneInstances, frameDeltaTime );
		UpdateInstanceList( cloudInstances, frameDeltaTime );

		std::vector<GfxAssetInstance> drawList = { { &shipRenderable, shipSceneInstance, false } };
		drawList.push_back( { &backgroundInstance.asset, backgroundInstance.instance, false } );
		for( SceneInstance& i : cloudInstances )
			drawList.push_back( { &cloudAsset, i, true } );
		for( BulletInstance& bi : bulletInstances )
			drawList.push_back( { &bulletRenderable, bi.sceneInstance, false } );
		for( EnemyShipInstance& enemyShipInstance : enemyShipSceneInstances )
			drawList.push_back( { &shipRenderable, enemyShipInstance.sceneInstance, false } );

		std::vector<TextZone> textZones = UpdateText();

		DrawFrame( current_frame, &cameraSceneInstance, drawList, textZones );

		current_frame = (++current_frame) % SIMULTANEOUS_FRAMES;
	}

	std::vector<SceneInstance> CreateClouds()
	{
		std::vector<SceneInstance> clouds;
		const float depth = 7.0f;

		clouds.push_back( { glm::vec3( 100.0f, 200.0f, depth ), defaultRotation, 20.0f } );
		clouds.push_back( { glm::vec3( 100.0f, 40.0f, depth ), defaultRotation, 20.0f } );
		clouds.push_back( { glm::vec3( 50.0f, 50.0f, depth ), defaultRotation, 20.0f } );
		clouds.push_back( { glm::vec3( -50.0f, -100.0f, depth ), defaultRotation, 20.0f } );

		return clouds;
	}

	void Init()
	{
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
		IH::BindInputToAction( "fire", IH::SPACE );

		//Console commands callback (need IH)
		ConCom::Init();

		//Objects update callbacks
		//RegisterTickFunction( &TickObjectCallback );

		//LoadAssets
		uint32_t memoryTypeMask = 15;/*TODO this works for now for textures, comes from asking VK for a textures*/
		gfx_heap = create_gfx_heap( 16 * 1024 * 1024, memoryTypeMask, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
		GfxHeaps_BatchedAllocator gfx_mem_allocator( &gfx_heap );
		gfx_mem_allocator.Prepare();

		GfxImage* shipTexture = AL::LoadTexture( "shipTexture", "assets/F14.png", &gfx_mem_allocator );
		GfxImage* bulletTexture = AL::LoadTexture( "bullet_texture", "assets/bullet_small.png", &gfx_mem_allocator );
		GfxImage* treeTexture = AL::LoadTexture( "tree_texture", "assets/tree.png", &gfx_mem_allocator );
		GfxImage* riverBankTexture = AL::LoadTexture( "river_bank_texture", "assets/river_bank.png", &gfx_mem_allocator );
		GfxImage* riverTexture = AL::LoadTexture( "river_texture", "assets/river.png", &gfx_mem_allocator );
		GfxImage* cloudTexture = AL::LoadTexture( "cloud_texture", "assets/cloud.png", &gfx_mem_allocator );
		GfxImage* background_sprite_sheet = AL::LoadTexture( "background_sprite_sheet", "assets/ground_spritesheet.png", &gfx_mem_allocator );		

		GfxModel* quadModel = AL::CreateQuad( "Quad", quadSize, &gfx_mem_allocator );

		uint32_t shipTextureIndex = RegisterBindlessTexture( &bindlessTexturesState, shipTexture, eSamplers::Point );
		uint32_t bulletTextureIndex = RegisterBindlessTexture( &bindlessTexturesState, bulletTexture, eSamplers::Point );
		uint32_t treeTextureIndex = RegisterBindlessTexture( &bindlessTexturesState, treeTexture, eSamplers::Point );
		uint32_t riverBankTextureIndex = RegisterBindlessTexture( &bindlessTexturesState, riverBankTexture, eSamplers::Point );
		uint32_t riverTextureIndex = RegisterBindlessTexture( &bindlessTexturesState, riverTexture, eSamplers::Point );
		uint32_t cloudTextureIndex = RegisterBindlessTexture( &bindlessTexturesState, cloudTexture, eSamplers::Point );
		uint32_t background_sprite_sheet_index = RegisterBindlessTexture( &bindlessTexturesState, background_sprite_sheet, eSamplers::Point );

		backgroundInstance = CreateBackgroundGfxModel( VIEWPORT_WIDTH, VIEWPORT_HEIGHT, background_sprite_sheet_index, &gfx_mem_allocator );

		gfx_mem_allocator.Commit();

		shipRenderable = CreateGfxAsset( quadModel, shipTextureIndex );
		bulletRenderable = CreateGfxAsset( quadModel, bulletTextureIndex );
		treeAsset = CreateGfxAsset( quadModel, treeTextureIndex );
		riverBankAsset = CreateGfxAsset( quadModel, riverBankTextureIndex );
		riverAsset = CreateGfxAsset( quadModel, riverTextureIndex );
		cloudAsset = CreateGfxAsset( quadModel, cloudTextureIndex );	

		CompileScene( &bindlessTexturesState );

		shipSceneInstance = { glm::vec3( 0.0f, 0.0f, 2.0f ), defaultRotation, shipSize };
		cameraSceneInstance = { glm::vec3( 0.0f, 0.0f, -2.0f ), defaultRotation, 1.0f };
		cloudInstances = CreateClouds();
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