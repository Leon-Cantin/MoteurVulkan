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

#include "entity.h"

namespace WildWeasel_Game
{
	uint32_t current_frame = 0;

	struct HealthComponent
	{
		float currentHealth;
		float maxHealth;
	};

	struct LifeTimeComponent
	{
		float lifeTime;
		float maxLifetime;
	};

	struct TransformationComponent
	{
		SceneInstance sceneInstance;
	};

	struct PhysicsComponent
	{
		glm::vec3 velocity;
	};

	struct TimeComponent
	{
		float deltaTime;
	};

	struct YKillComponent
	{
		float yKill;
	};

	struct CollisionComponent
	{
		float rayLenght;
	};

	struct RenderableComponent
	{
		GfxAsset* asset;
		bool dithering;
	};

	struct DamageComponent
	{
		float damage;
	};

	struct DrawlistComponent
	{
		std::vector<GfxAssetInstance> drawlist;
	};

	auto t = ECS::RegisterComponentType<HealthComponent>::comp;
	auto t1 = ECS::RegisterComponentType<LifeTimeComponent>::comp;
	auto t2 = ECS::RegisterComponentType<TransformationComponent>::comp;
	auto t4 = ECS::RegisterComponentType<PhysicsComponent>::comp;
	auto t6 = ECS::RegisterComponentType<CollisionComponent>::comp;
	auto t7 = ECS::RegisterComponentType<RenderableComponent>::comp;
	auto t9 = ECS::RegisterComponentType<DamageComponent>::comp;

	auto t3 = ECS::RegisterSingletonComponentType<TimeComponent>::comp;
	auto t5 = ECS::RegisterSingletonComponentType<YKillComponent>::comp;
	auto t8 = ECS::RegisterSingletonComponentType<DrawlistComponent>::comp;

	ECS::EntityComponentSystem ecs;

	void UpdateLifeTime( ECS::Entity* entities, uint32_t count, ECS::EntityComponentSystem* ecs )
	{
		for( uint32_t i = 0; i < count; ++i )
		{
			ECS::Entity& entity = entities[i];
			LifeTimeComponent* lifeTimeComp = entity.GetComponent<LifeTimeComponent>();
			const TimeComponent* timeComponent = ecs->GetSingletonComponent<TimeComponent>();
			lifeTimeComp->lifeTime += (timeComponent->deltaTime / 1000.0f);
			if( lifeTimeComp->lifeTime >= lifeTimeComp->maxLifetime )
			{
				ecs->Destroy( &entity );
				continue;
			}

			const TransformationComponent* transformationComponent = entity.GetComponent<TransformationComponent>();
			const YKillComponent* yKillComponent = ecs->GetSingletonComponent<YKillComponent>();
			if( abs( transformationComponent->sceneInstance.location.y ) > yKillComponent->yKill )
			{
				ecs->Destroy( &entity );
				continue;
			}
		}
	}

	void UpdatePhysics( ECS::Entity* entities, uint32_t count, ECS::EntityComponentSystem* ecs )
	{
		const TimeComponent* timeComponent = ecs->GetSingletonComponent<TimeComponent>();
		for( uint32_t i = 0; i < count; ++i )
		{
			const ECS::Entity& entity = entities[i];
			PhysicsComponent* physicsComponent = entity.GetComponent<PhysicsComponent>();
			TransformationComponent* transformationComponent = entity.GetComponent<TransformationComponent>();

			transformationComponent->sceneInstance.location += physicsComponent->velocity * (timeComponent->deltaTime / 1000.0f);
		}
	}

	void UpdateCollisions( ECS::Entity* entities, uint32_t count, ECS::EntityComponentSystem* ecs )
	{
		for( uint32_t i = 0; i < count; ++i )
		{
			ECS::Entity& entity = entities[i];

			if( !entity.IsValid() )//Entities could have already been destroyed by another collision
				continue;

			const CollisionComponent* collisionComponent = entity.GetComponent<CollisionComponent>();
			const TransformationComponent* transformationComponent = entity.GetComponent<TransformationComponent>();

			//TODO: use a collision filter ID and bitmask
			for( uint32_t j = i+1; j < count; ++j )
			{
				ECS::Entity& otherEntity = entities[j];

				if( !otherEntity.IsValid() )//Entities could have already been destroyed by another collision
					continue;

				const CollisionComponent* otherCollisionComponent = otherEntity.GetComponent<CollisionComponent>();
				const TransformationComponent* otherTransformationComponent = otherEntity.GetComponent<TransformationComponent>();
				
				const float finalRay = collisionComponent->rayLenght + otherCollisionComponent->rayLenght;
				const float dx = abs( otherTransformationComponent->sceneInstance.location.x - transformationComponent->sceneInstance.location.x );
				const float dy = abs( otherTransformationComponent->sceneInstance.location.y - transformationComponent->sceneInstance.location.y );
				if( sqrt( dx * dx + dy * dy ) < finalRay )
				{
					const DamageComponent* damageComponent = entity.GetComponent<DamageComponent>();
					HealthComponent* healthComponent = entity.GetComponent<HealthComponent>();
					const DamageComponent* otherDamageComponent = otherEntity.GetComponent<DamageComponent>();
					HealthComponent* otherHealthComponent = otherEntity.GetComponent<HealthComponent>();

					if( damageComponent && otherHealthComponent )
						otherHealthComponent->currentHealth -= damageComponent->damage;

					if( otherDamageComponent && healthComponent )
						healthComponent->currentHealth -= otherDamageComponent->damage;

					if( damageComponent || ( healthComponent && healthComponent->currentHealth <= 0 ) )
						ecs->Destroy( &entity );

					if( otherDamageComponent || (otherHealthComponent && otherHealthComponent->currentHealth <= 0 ) )
						ecs->Destroy( &otherEntity );
				}
			}
		}
	}

	void BuildDrawlistSystem( ECS::Entity* entities, uint32_t count, ECS::EntityComponentSystem* ecs )
	{
		for( uint32_t i = 0; i < count; ++i )
		{
			const ECS::Entity& entity = entities[i];
			//TODO: ordering
			RenderableComponent* renderableComponent = entity.GetComponent<RenderableComponent>();
			TransformationComponent* transformationComponent = entity.GetComponent<TransformationComponent>();
			DrawlistComponent* drawlistComponent = ecs->GetSingletonComponent<DrawlistComponent>();

			drawlistComponent->drawlist.push_back( { renderableComponent->asset, transformationComponent->sceneInstance, renderableComponent->dithering } );
		}
	}

	ECS::System lifeTimeSystem = ECS::System::Create<LifeTimeComponent, TransformationComponent>( UpdateLifeTime );
	ECS::System physicsSystem = ECS::System::Create<PhysicsComponent, TransformationComponent>( UpdatePhysics );
	ECS::System collisionsSystem = ECS::System::Create<CollisionComponent, TransformationComponent>( UpdateCollisions );
	ECS::System buildDrawlistSystem = ECS::System::Create < RenderableComponent, TransformationComponent >( BuildDrawlistSystem );

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

	BackgroundInstance backgroundInstance;
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

	constexpr float shipSize = 21.0f;

	void CreateEnemyShip()
	{
		const float x = ((float)std::rand()/ RAND_MAX) * 150.0f -75.0f;
		constexpr float y = 190.0f;

		auto hLifetimeComp = ecs.CreateComponent<LifeTimeComponent>( 0.0f, 5.0f );
		auto hHealthComp = ecs.CreateComponent<HealthComponent>( 5.0f, 5.0f );
		auto hRenderableComp = ecs.CreateComponent<RenderableComponent>( &shipRenderable, false );
		auto hTransformationComp = ecs.CreateComponent<TransformationComponent>( glm::vec3( x, y, 2.0f ), defaultRotation, -shipSize );
		auto hPhysicsComp = ecs.CreateComponent<PhysicsComponent>( glm::vec3( 0.0f, -100.0f, 0.0f ) );
		auto hCollisionComp = ecs.CreateComponent<CollisionComponent>( 7.0f );

		ecs.CreateEntity( hLifetimeComp, hHealthComp, hRenderableComp, hTransformationComp, hPhysicsComp, hCollisionComp );
	}

	void CreateCloud()
	{
		const float x = (( float )std::rand() / RAND_MAX) * 150.0f - 75.0f;
		const float size = (( float )std::rand() / RAND_MAX) * 10.0f + 10.0f;
		constexpr float y = 190.0f;

		auto hLifetimeComp = ecs.CreateComponent<LifeTimeComponent>( 0.0f, 5.0f );
		auto hRenderableComp = ecs.CreateComponent<RenderableComponent>( &cloudAsset, true );
		auto hTransformationComp = ecs.CreateComponent<TransformationComponent>( glm::vec3( x, y, 7.0f ), defaultRotation, size );
		auto hPhysicsComp = ecs.CreateComponent<PhysicsComponent>( glm::vec3( 0.0f, -100.0f, 0.0f ) );

		ecs.CreateEntity( hLifetimeComp, hRenderableComp, hTransformationComp, hPhysicsComp );
	}

	void createBullet()
	{
		constexpr float bulletSpeedPerSecond = 150.0f;
		constexpr float collisionSphereRayLenght = 10.0f;
		constexpr float bulletSize = 7.0f;
		static bool left = false;
		left ^= true;
		const float bullet_offset = 2.0f;
		const float xoffset = left ? -bullet_offset : bullet_offset;
		const glm::vec3 offset( xoffset, 0.0f, 0.0f );

		auto hLifetimeComp = ecs.CreateComponent<LifeTimeComponent>( 0.0f, 5.0f );
		auto hRenderableComp = ecs.CreateComponent<RenderableComponent>( &bulletRenderable, false );
		auto hTransformationComp = ecs.CreateComponent<TransformationComponent>( shipSceneInstance.location + offset, shipSceneInstance.orientation, bulletSize );
		auto hPhysicsComp = ecs.CreateComponent<PhysicsComponent>( glm::vec3( 0.0f, 150.0f, 0.0f ) );
		auto hCollisionComp = ecs.CreateComponent<CollisionComponent>( 3.0f );
		auto hDamageComp = ecs.CreateComponent<DamageComponent>( 1.0f );

		ecs.CreateEntity( hLifetimeComp, hRenderableComp, hTransformationComp, hPhysicsComp, hCollisionComp, hDamageComp );
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

		TimeComponent* timeComponent = ecs.GetSingletonComponent<TimeComponent>();
		timeComponent->deltaTime = frameDeltaTime;
		YKillComponent* ykillComponent = ecs.GetSingletonComponent<YKillComponent>();
		ykillComponent->yKill = 200.0f;

		ecs.RunSystem( lifeTimeSystem );
		ecs.RunSystem( physicsSystem );
		ecs.RunSystem( collisionsSystem );

		DrawlistComponent* drawlistComponent = ecs.GetSingletonComponent<DrawlistComponent>();
		drawlistComponent->drawlist.clear();
		drawlistComponent->drawlist.push_back( { &shipRenderable, shipSceneInstance, false } );
		drawlistComponent->drawlist.push_back( { &backgroundInstance.asset, backgroundInstance.instance, false } );
		ecs.RunSystem( buildDrawlistSystem );

		std::vector<TextZone> textZones = UpdateText();

		DrawFrame( current_frame, &cameraSceneInstance, drawlistComponent->drawlist, textZones );

		current_frame = (++current_frame) % SIMULTANEOUS_FRAMES;
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

		ecs.CreateSingletonComponent<TimeComponent>();
		ecs.CreateSingletonComponent<YKillComponent>( 200.0f );
		ecs.CreateSingletonComponent<DrawlistComponent>();
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