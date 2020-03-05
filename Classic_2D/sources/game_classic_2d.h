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

	const int VIEWPORT_WIDTH = 224;
	const int VIEWPORT_HEIGHT = 384;
	const int SCREEN_SCALE = 2;
	const int SCREEN_WIDTH = VIEWPORT_WIDTH * SCREEN_SCALE;
	const int SCREEN_HEIGHT = VIEWPORT_HEIGHT * SCREEN_SCALE;

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

	struct BackgroundInstance
	{
		SceneInstance instance;
		GfxAsset* asset;
	};

	std::vector<BulletInstance> bulletInstances;
	std::vector<EnemyShipInstance> enemyShipSceneInstances;
	std::vector<BackgroundInstance> backgroundInstances;
	std::vector<SceneInstance> cloudInstances;
	SceneInstance shipSceneInstance;
	SceneInstance cameraSceneInstance;

	GfxAsset shipRenderable;
	GfxAsset bulletRenderable;
	GfxAsset treeAsset;
	GfxAsset riverBankAsset;
	GfxAsset riverAsset;
	GfxAsset cloudAsset;
	GfxAsset background_sprite_sheet_asset;

	BindlessTexturesState bindlessTexturesState;

	size_t frameDeltaTime = 0;
	uint32_t _score = 0;

	glm::fquat defaultRotation = glm::angleAxis( glm::radians( 0.0f ), glm::vec3 { 0.0f, 1.0f, 0.0f } );
	constexpr float quadSize = 1.0f;

	GfxHeap imagesHeap;

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

	void UpdateBackgroundScrolling( float frame_delta_time)
	{
		const float movement_speed = 200.0f;
		backgroundInstances[0].instance.location.y -= movement_speed * (frame_delta_time / 1000.0f);
		if( backgroundInstances[0].instance.location.y < ( -VIEWPORT_HEIGHT / 2.0f - 20.0f ) )
			backgroundInstances[0].instance.location.y = -VIEWPORT_HEIGHT / 2.0f;
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

			static size_t lastSpawn = 0;
			if( lastSpawn == 0 || currentTime - lastSpawn > 1000 )
			{
				CreateEnemyShip();
				CreateCloud();
				lastSpawn = currentTime;
			}

			UpdateBackgroundScrolling( frameDeltaTime );

			//Update objects
			//TickUpdate( frameDeltaTime );

			UpdateInstanceList( bulletInstances, frameDeltaTime );
			UpdateInstanceList( enemyShipSceneInstances, frameDeltaTime );
			UpdateInstanceList( cloudInstances, frameDeltaTime );

			std::vector<GfxAssetInstance> drawList = { { &shipRenderable, shipSceneInstance, false } };
			for( BackgroundInstance& i : backgroundInstances )
				drawList.push_back( { i.asset, i.instance, false } );
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

		vkDeviceWaitIdle(g_vk.device);
	}

	GfxAsset CreateGfxAsset( const GfxModel* modelAsset, uint32_t albedoIndex )
	{
		GfxAsset asset = { modelAsset, { albedoIndex } };
		return asset;
	}

	static void GenerateQuadMegaTextureUVs( const uint32_t x_index, const uint32_t y_index, const uint32_t num_sprites_x, const uint32_t num_sprites_y,
		glm::vec2* ll, glm::vec2* lr, glm::vec2* ur, glm::vec2* ul )
	{
		float left = 1.0f / ( float )num_sprites_x * ( float )x_index;
		float right = 1.0f / ( float )num_sprites_x * ( float )(x_index + 1);
		float upper = 1.0f / ( float )num_sprites_y * ( float )y_index;
		float lower = 1.0f / ( float )num_sprites_y * ( float )(y_index + 1);
		*ll = { left, lower };
		*lr = { right, lower };
		*ur = { right, upper };
		*ul = { left, upper };
	}

	typedef uint32_t Index_t;
	GfxModel CreateBackgroundGfxModel( const uint32_t screen_width, const uint32_t screen_height, I_BufferAllocator* allocator )
	{
		const unsigned int sprite_size = 20;
		const unsigned int num_sprites_x = 2;
		const unsigned int num_sprites_y = 2;
		const unsigned int river_bank_index = 0;
		const unsigned int river_index = 1;
		const unsigned int forest_index = 2;
		const float depth = 8.0f;
		const float offset = 1.0f;//Let's all have them 1 unit and scale at render time
		const int tiles_count_x = screen_width / sprite_size + 1;
		const int tiles_count_y = screen_height / sprite_size + 2;

		const unsigned int vertices_per_quad = 4;
		const unsigned int total_vertices = vertices_per_quad * tiles_count_x * tiles_count_y;
		std::vector<glm::vec3> vertices_pos;
		vertices_pos.resize( total_vertices );
		std::vector<glm::vec3> vertices_color;
		vertices_color.resize( total_vertices );
		std::vector<glm::vec2> vertices_uv;
		vertices_uv.resize( total_vertices );

		const unsigned int index_per_quad = 6;
		const unsigned int total_indices = index_per_quad * tiles_count_x * tiles_count_y;
		std::vector<Index_t> indices;
		indices.resize( total_indices );

		for( unsigned int y = 0; y < tiles_count_y; ++y )
		{
			for( unsigned int x = 0; x < tiles_count_x; ++x )
			{
				uint32_t mega_texture_index = forest_index;
				if( x > 2 )
					mega_texture_index = river_bank_index;
				if( x > 3 )
					mega_texture_index = river_index;

				const unsigned int mega_texture_y_index = mega_texture_index / num_sprites_x;
				const unsigned int mega_texture_x_index = mega_texture_index % num_sprites_x;

				const unsigned int array_offset = (x + tiles_count_x * y);
				const unsigned int vertices_array_offset = array_offset * vertices_per_quad;//TODO: probably wrapping around on y reset
				vertices_pos[vertices_array_offset + 0] = { x*offset, y*offset, 0.0f };
				vertices_pos[vertices_array_offset + 1] = { x*offset + offset, y*offset, 0.0f };
				vertices_pos[vertices_array_offset + 2] = { x*offset + offset, y*offset + offset, 0.0f };
				vertices_pos[vertices_array_offset + 3] = { x*offset, y*offset + offset, 0.0f };

				vertices_color[vertices_array_offset + 0] = { 1.0f, 0.0f, 0.0f };
				vertices_color[vertices_array_offset + 1] = { 0.0f, 1.0f, 0.0f };
				vertices_color[vertices_array_offset + 2] = { 0.0f, 0.0f, 1.0f };
				vertices_color[vertices_array_offset + 3] = { 1.0f, 1.0f, 1.0f };

				GenerateQuadMegaTextureUVs( mega_texture_x_index, mega_texture_y_index, num_sprites_x, num_sprites_y,
					&vertices_uv[vertices_array_offset + 0],
					&vertices_uv[vertices_array_offset + 1],
					&vertices_uv[vertices_array_offset + 2],
					&vertices_uv[vertices_array_offset + 3]
					);

				const unsigned int index_array_offset = array_offset * index_per_quad;
				indices[index_array_offset + 0] = vertices_array_offset + 0;
				indices[index_array_offset + 1] = vertices_array_offset + 1;
				indices[index_array_offset + 2] = vertices_array_offset + 2;
				indices[index_array_offset + 3] = vertices_array_offset + 2;
				indices[index_array_offset + 4] = vertices_array_offset + 3;
				indices[index_array_offset + 5] = vertices_array_offset + 0;
			}
		}

		std::vector<VIDesc> modelVIDescs = {
			{ eVIDataType::POSITION, eVIDataElementType::FLOAT, 3 },
			{ eVIDataType::COLOR, eVIDataElementType::FLOAT, 3 },
			{ eVIDataType::TEX_COORD, eVIDataElementType::FLOAT, 2 },
		};

		std::vector<void*> modelData = {
			vertices_pos.data(),
			vertices_color.data(),
			vertices_uv.data(),
		};

		return CreateGfxModel( modelVIDescs, modelData, total_vertices, indices.data(), total_indices, sizeof( Index_t ), allocator );
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
		WH::InitializeWindow( SCREEN_WIDTH, SCREEN_HEIGHT, "Wild Weasel: Vietnam" );
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
		IH::BindInputToAction( "fire", IH::SPACE );

		//Console commands callback (need IH)
		ConCom::Init();

		//Objects update callbacks
		//RegisterTickFunction( &TickObjectCallback );

		//Init renderer stuff
		InitRendererImp( WH::VK::_windowSurface );

		//LoadAssets
		uint32_t memoryTypeMask = 15;/*TODO this works for now for textures, comes from asking VK for a textures*/
		imagesHeap = create_gfx_heap( 16 * 1024 * 1024, memoryTypeMask, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
		GfxHeaps_BatchedAllocator imagesAllocator( &imagesHeap );
		imagesAllocator.Prepare();		
		GfxImage* shipTexture = AL::LoadTexture( "shipTexture", "assets/F14.png", &imagesAllocator );
		GfxImage* bulletTexture = AL::LoadTexture( "bullet_texture", "assets/bullet_small.png", &imagesAllocator );
		GfxImage* treeTexture = AL::LoadTexture( "tree_texture", "assets/tree.png", &imagesAllocator );
		GfxImage* riverBankTexture = AL::LoadTexture( "river_bank_texture", "assets/river_bank.png", &imagesAllocator );
		GfxImage* riverTexture = AL::LoadTexture( "river_texture", "assets/river.png", &imagesAllocator );
		GfxImage* cloudTexture = AL::LoadTexture( "cloud_texture", "assets/cloud.png", &imagesAllocator );
		GfxImage* background_sprite_sheet = AL::LoadTexture( "background_sprite_sheet", "assets/ground_spritesheet.png", &imagesAllocator );		

		GfxModel* quadModel = AL::CreateQuad( "Quad", quadSize, &imagesAllocator );
		GfxModel* backgroundModel = AL::RegisterGfxModel( "background", CreateBackgroundGfxModel( VIEWPORT_WIDTH, VIEWPORT_HEIGHT, &imagesAllocator ));
		imagesAllocator.Commit();

		uint32_t shipTextureIndex = RegisterBindlessTexture( &bindlessTexturesState, shipTexture, eSamplers::Point );
		uint32_t bulletTextureIndex = RegisterBindlessTexture( &bindlessTexturesState, bulletTexture, eSamplers::Point );
		uint32_t treeTextureIndex = RegisterBindlessTexture( &bindlessTexturesState, treeTexture, eSamplers::Point );
		uint32_t riverBankTextureIndex = RegisterBindlessTexture( &bindlessTexturesState, riverBankTexture, eSamplers::Point );
		uint32_t riverTextureIndex = RegisterBindlessTexture( &bindlessTexturesState, riverTexture, eSamplers::Point );
		uint32_t cloudTextureIndex = RegisterBindlessTexture( &bindlessTexturesState, cloudTexture, eSamplers::Point );
		uint32_t background_sprite_sheet_index = RegisterBindlessTexture( &bindlessTexturesState, background_sprite_sheet, eSamplers::Point );

		shipRenderable = CreateGfxAsset( quadModel, shipTextureIndex );
		bulletRenderable = CreateGfxAsset( quadModel, bulletTextureIndex );
		treeAsset = CreateGfxAsset( quadModel, treeTextureIndex );
		riverBankAsset = CreateGfxAsset( quadModel, riverBankTextureIndex );
		riverAsset = CreateGfxAsset( quadModel, riverTextureIndex );
		cloudAsset = CreateGfxAsset( quadModel, cloudTextureIndex );	
		background_sprite_sheet_asset = CreateGfxAsset( backgroundModel, background_sprite_sheet_index );

		CompileScene( &bindlessTexturesState );

		shipSceneInstance = { glm::vec3( 0.0f, 0.0f, 2.0f ), defaultRotation, shipSize };
		cameraSceneInstance = { glm::vec3( 0.0f, 0.0f, -2.0f ), defaultRotation, 1.0f };
		backgroundInstances.push_back( { { glm::vec3( -VIEWPORT_WIDTH/2.0f, -VIEWPORT_HEIGHT/2.0f, 8.0f ), defaultRotation, 20.0f }, &background_sprite_sheet_asset } );
		cloudInstances = CreateClouds();
	}

	void cleanup() 
	{
		CleanupRendererImp();

		AL::Cleanup();

		destroy( &imagesHeap );
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