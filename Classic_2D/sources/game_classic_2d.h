#pragma once

#include "classic_2d_renderer_imp.h"
#include "console_command.h"
#include "input.h"
#include "tick_system.h"
#include "asset_library.h"
#include "window_handler_vk.h"
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

	SceneInstance shipSceneInstance;
	SceneInstance cameraSceneInstance;

	GfxAsset shipAsset;
	GfxAsset bulletRenderable;
	GfxAsset cloudAsset;
	GfxAsset mig19Asset;
	GfxAsset rescueAsset;
	GfxAsset enemySoldierAsset;

	BindlessTexturesState bindlessTexturesState;

	size_t frameDeltaTime = 0;
	uint32_t _score = 0;

	GfxHeap gfx_heap;
	GfxHeap gfx_heap_host_visible;

	constexpr float shipSize = 21.0f;
	constexpr float soldierSize = 9.0f;

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
		float currentTime;
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

	struct EnemyShipSpawnerComponent
	{
		int timeUntilNextSpawn = 0;
	};

	struct EnvironmentComponent
	{
		int timeUntilNextCloudSpawn = 0;
	};

	struct BackgroundInstanceComponent
	{
		BackgroundInstance instance;
	};

	using ScriptCallback_t = void( *)( ECS::Entity* entity, ECS::EntityComponentSystem* ecs );
	struct ScriptComponent
	{
		ScriptCallback_t scriptCallback;
	};

	REGISTER_COMPONENT_TYPE( HealthComponent );
	REGISTER_COMPONENT_TYPE( LifeTimeComponent );
	REGISTER_COMPONENT_TYPE( TransformationComponent );
	REGISTER_COMPONENT_TYPE( PhysicsComponent );
	REGISTER_COMPONENT_TYPE( CollisionComponent );
	REGISTER_COMPONENT_TYPE( RenderableComponent );
	REGISTER_COMPONENT_TYPE( DamageComponent );
	REGISTER_COMPONENT_TYPE( BackgroundInstanceComponent );
	REGISTER_COMPONENT_TYPE( ScriptComponent );
	REGISTER_COMPONENT_TYPE( EnemyShipSpawnerComponent );
	REGISTER_COMPONENT_TYPE( EnvironmentComponent );

	REGISTER_SINGLETON_COMPONENT_TYPE( TimeComponent );
	REGISTER_SINGLETON_COMPONENT_TYPE( YKillComponent );
	REGISTER_SINGLETON_COMPONENT_TYPE( DrawlistComponent );

	ECS::EntityComponentSystem ecs;

	void UpdateBackgroundScript( ECS::Entity* entity, ECS::EntityComponentSystem* ecs )
	{
		BackgroundInstanceComponent* backgroundComp = entity->GetComponent<BackgroundInstanceComponent>();
		const TimeComponent* timeComp = ecs->GetSingletonComponent< TimeComponent>();
		UpdateBackgroundScrolling( &backgroundComp->instance, timeComp->deltaTime );

		//TODO don't call it like this ...
		RenderableComponent* renderableComp = entity->GetComponent<RenderableComponent>();
		renderableComp->asset = &backgroundComp->instance.asset;

		//TODO don't call it like this ...
		TransformationComponent* transformationComp = entity->GetComponent<TransformationComponent>();
		transformationComp->sceneInstance = backgroundComp->instance.instance;
	}

	void UpdateShipScript( ECS::Entity* entity, ECS::EntityComponentSystem* ecs )
	{
		//TODO: can I instead read the inputs and update the component directly?
		TransformationComponent* transformationComp = entity->GetComponent<TransformationComponent>();
		transformationComp->sceneInstance = shipSceneInstance;
	}

	void RunEntityScripts( ECS::Entity* entities, uint32_t count, ECS::EntityComponentSystem* ecs )
	{
		for( uint32_t i = 0; i < count; ++i )
		{
			ECS::Entity& entity = entities[i];
			const ScriptComponent* scriptComp = entity.GetComponent<ScriptComponent>();
			scriptComp->scriptCallback( &entity, ecs );
		}
	}

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

			//TODO: use a collision filter ID and bitmask kinda like UE4
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
		DrawlistComponent* drawlistComponent = ecs->GetSingletonComponent<DrawlistComponent>();
		drawlistComponent->drawlist.clear();

		for( uint32_t i = 0; i < count; ++i )
		{
			const ECS::Entity& entity = entities[i];
			//TODO: ordering
			RenderableComponent* renderableComponent = entity.GetComponent<RenderableComponent>();
			TransformationComponent* transformationComponent = entity.GetComponent<TransformationComponent>();			

			drawlistComponent->drawlist.push_back( { renderableComponent->asset, transformationComponent->sceneInstance, renderableComponent->dithering } );
		}
	}

	void CreateEnemyShipScript( ECS::Entity* entity, ECS::EntityComponentSystem* ecs )
	{
		const TimeComponent* timeComponent = ecs->GetSingletonComponent<TimeComponent>();
		EnemyShipSpawnerComponent* spawnerComponent = entity->GetComponent<EnemyShipSpawnerComponent>();

		if( ( spawnerComponent->timeUntilNextSpawn -= static_cast<int>(timeComponent->deltaTime) ) < 0 )
		{
			const float x = (( float )std::rand() / RAND_MAX) * 150.0f - 75.0f;
			constexpr float y = 190.0f;

			auto hLifetimeComp = ecs->CreateComponent<LifeTimeComponent>( 0.0f, 5.0f );
			auto hHealthComp = ecs->CreateComponent<HealthComponent>( 5.0f, 5.0f );
			auto hRenderableComp = ecs->CreateComponent<RenderableComponent>( &mig19Asset, false );
			auto hTransformationComp = ecs->CreateComponent<TransformationComponent>( glm::vec3( x, y, 2.0f ), defaultRotation, -shipSize );
			auto hPhysicsComp = ecs->CreateComponent<PhysicsComponent>( glm::vec3( 0.0f, -100.0f, 0.0f ) );
			auto hCollisionComp = ecs->CreateComponent<CollisionComponent>( 7.0f );

			ecs->CreateEntity( hLifetimeComp, hHealthComp, hRenderableComp, hTransformationComp, hPhysicsComp, hCollisionComp );

			spawnerComponent->timeUntilNextSpawn = 1000;
		}
	}

	void CreateEnemySoldierScript( ECS::Entity* entity, ECS::EntityComponentSystem* ecs )
	{
		const TimeComponent* timeComponent = ecs->GetSingletonComponent<TimeComponent>();
		EnemyShipSpawnerComponent* spawnerComponent = entity->GetComponent<EnemyShipSpawnerComponent>();

		if( (spawnerComponent->timeUntilNextSpawn -= static_cast< int >(timeComponent->deltaTime)) < 0 )
		{
			const float x = (( float )std::rand() / RAND_MAX) * 150.0f - 75.0f;
			constexpr float y = 190.0f;

			auto hLifetimeComp = ecs->CreateComponent<LifeTimeComponent>( 0.0f, 10.0f );
			auto hHealthComp = ecs->CreateComponent<HealthComponent>( 2.0f, 2.0f );
			auto hRenderableComp = ecs->CreateComponent<RenderableComponent>( &enemySoldierAsset, false );
			auto hTransformationComp = ecs->CreateComponent<TransformationComponent>( glm::vec3( x, y, 2.0f ), defaultRotation, -soldierSize );
			auto hPhysicsComp = ecs->CreateComponent<PhysicsComponent>( glm::vec3( 0.0f, -50.0f, 0.0f ) );
			auto hCollisionComp = ecs->CreateComponent<CollisionComponent>( 7.0f );

			ecs->CreateEntity( hLifetimeComp, hHealthComp, hRenderableComp, hTransformationComp, hPhysicsComp, hCollisionComp );

			spawnerComponent->timeUntilNextSpawn = 1000;
		}
	}

	void CreateRescueScript( ECS::Entity* entity, ECS::EntityComponentSystem* ecs )
	{
		const TimeComponent* timeComponent = ecs->GetSingletonComponent<TimeComponent>();
		EnemyShipSpawnerComponent* spawnerComponent = entity->GetComponent<EnemyShipSpawnerComponent>();

		if( (spawnerComponent->timeUntilNextSpawn -= static_cast< int >(timeComponent->deltaTime)) < 0 )
		{
			const float x = (( float )std::rand() / RAND_MAX) * 150.0f - 75.0f;
			constexpr float y = 190.0f;

			auto hLifetimeComp = ecs->CreateComponent<LifeTimeComponent>( 0.0f, 5.0f );
			auto hRenderableComp = ecs->CreateComponent<RenderableComponent>( &rescueAsset, false );
			auto hTransformationComp = ecs->CreateComponent<TransformationComponent>( glm::vec3( x, y, 2.0f ), defaultRotation, -soldierSize );
			auto hPhysicsComp = ecs->CreateComponent<PhysicsComponent>( glm::vec3( 0.0f, -50.0f, 0.0f ) );
			auto hCollisionComp = ecs->CreateComponent<CollisionComponent>( 7.0f );

			ecs->CreateEntity( hLifetimeComp, hRenderableComp, hTransformationComp, hPhysicsComp, hCollisionComp );

			spawnerComponent->timeUntilNextSpawn = 10000;
		}
	}

	void CreateCloudScript( ECS::Entity* entity, ECS::EntityComponentSystem* ecs )
	{
		const TimeComponent* timeComponent = ecs->GetSingletonComponent<TimeComponent>();
		EnvironmentComponent* spawnerComponent = entity->GetComponent<EnvironmentComponent>();

		if( (spawnerComponent->timeUntilNextCloudSpawn -= static_cast< int >(timeComponent->deltaTime)) < 0 )
		{
			const float x = (( float )std::rand() / RAND_MAX) * 150.0f - 75.0f;
			const float size = (( float )std::rand() / RAND_MAX) * 10.0f + 10.0f;
			constexpr float y = 190.0f;

			auto hLifetimeComp = ecs->CreateComponent<LifeTimeComponent>( 0.0f, 5.0f );
			auto hRenderableComp = ecs->CreateComponent<RenderableComponent>( &cloudAsset, true );
			auto hTransformationComp = ecs->CreateComponent<TransformationComponent>( glm::vec3( x, y, 7.0f ), defaultRotation, size );
			auto hPhysicsComp = ecs->CreateComponent<PhysicsComponent>( glm::vec3( 0.0f, -100.0f, 0.0f ) );

			ecs->CreateEntity( hLifetimeComp, hRenderableComp, hTransformationComp, hPhysicsComp );

			spawnerComponent->timeUntilNextCloudSpawn = 1500;
		}
	}

	ECS::System lifeTimeSystem = ECS::System::Create<LifeTimeComponent, TransformationComponent>( UpdateLifeTime );
	ECS::System physicsSystem = ECS::System::Create<PhysicsComponent, TransformationComponent>( UpdatePhysics );
	ECS::System collisionsSystem = ECS::System::Create<CollisionComponent, TransformationComponent>( UpdateCollisions );
	ECS::System buildDrawlistSystem = ECS::System::Create < RenderableComponent, TransformationComponent >( BuildDrawlistSystem );
	ECS::System runEntityScriptsSystem = ECS::System::Create< ScriptComponent >( RunEntityScripts );

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
		int charCount = snprintf( textBuffer, 16, "Score: %i", _score );
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

		//Run Ecs
		TimeComponent* timeComponent = ecs.GetSingletonComponent<TimeComponent>();
		timeComponent->deltaTime = frameDeltaTime;
		YKillComponent* ykillComponent = ecs.GetSingletonComponent<YKillComponent>();
		ykillComponent->yKill = 200.0f;

		ecs.RunSystem( lifeTimeSystem );
		ecs.RunSystem( physicsSystem );
		ecs.RunSystem( collisionsSystem );

		ecs.RunSystem( runEntityScriptsSystem );

		ecs.RunSystem( buildDrawlistSystem );

		//Draw
		std::vector<TextZone> textZones = UpdateText();

		DrawlistComponent* drawlistComponent = ecs.GetSingletonComponent<DrawlistComponent>();
		DrawFrame( current_frame, &cameraSceneInstance, drawlistComponent->drawlist, textZones );

		current_frame = (++current_frame) % SIMULTANEOUS_FRAMES;
	}

	void CreateBackgroundEntity( uint32_t background_sprite_sheet_index, I_BufferAllocator* gfx_mem_allocator )
	{
		auto hBackgroundComp = ecs.CreateComponent<BackgroundInstanceComponent>( CreateBackgroundGfxModel( VIEWPORT_WIDTH, VIEWPORT_HEIGHT, background_sprite_sheet_index, gfx_mem_allocator ) );
		auto hTransormationComp = ecs.CreateComponent<TransformationComponent>();
		auto hRenderableComp = ecs.CreateComponent<RenderableComponent>();
		auto hScriptComp = ecs.CreateComponent<ScriptComponent>( UpdateBackgroundScript );
		ecs.CreateEntity( hBackgroundComp, hTransormationComp, hRenderableComp, hScriptComp );
	}

	void Init()
	{		
		//Input callbacks
		IH::InitInputs();
		IH::RegisterAction( "Forward", IH::W );
		IH::BindAction( "Forward", IH::Pressed, &ForwardCallback );
		IH::RegisterAction( "backward", IH::S );
		IH::BindAction( "backward", IH::Pressed, &BackwardCallback );
		IH::RegisterAction( "left", IH::A );
		IH::BindAction( "left", IH::Pressed, &MoveLeftCallback );
		IH::RegisterAction( "right", IH::D );
		IH::BindAction( "right", IH::Pressed, &MoveRightCallback );
		IH::RegisterAction( "fire", IH::SPACE );
		IH::BindAction( "fire", IH::Pressed, &FireCallback );

		IH::RegisterAction( "console", IH::TILD );
		IH::BindAction( "console", IH::Pressed, &ConCom::OpenConsole );


		//Console commands callback (need IH)
		ConCom::Init();

		//LoadAssets
		gfx_heap = create_gfx_heap( 16 * 1024 * 1024, GFX_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
		GfxHeaps_BatchedAllocator gfx_mem_allocator( &gfx_heap );
		gfx_mem_allocator.Prepare();

		gfx_heap_host_visible = create_gfx_heap( 16 * 1024 * 1024, GFX_MEMORY_PROPERTY_HOST_VISIBLE_BIT );
		GfxHeaps_Allocator gfx_host_visible_mem_allocator( &gfx_heap_host_visible );


		GfxImage* shipTexture = AL::LoadTexture( "shipTexture", "assets/uh1.png", &gfx_mem_allocator );
		GfxImage* mig19Texture = AL::LoadTexture( "mig_21_texture", "assets/Mig21.png", &gfx_mem_allocator );
		GfxImage* bulletTexture = AL::LoadTexture( "bullet_texture", "assets/bullet_small.png", &gfx_mem_allocator );
		GfxImage* cloudTexture = AL::LoadTexture( "cloud_texture", "assets/cloud.png", &gfx_mem_allocator );
		GfxImage* background_sprite_sheet = AL::LoadTexture( "background_sprite_sheet", "assets/ground_spritesheet.png", &gfx_mem_allocator );		
		GfxImage* rescueTexture = AL::LoadTexture( "rescue_texture", "assets/rescue.png", &gfx_mem_allocator );

		GfxModel* quadModel = AL::CreateQuad( "Quad", quadSize, &gfx_mem_allocator );

		uint32_t shipTextureIndex = RegisterBindlessTexture( &bindlessTexturesState, shipTexture, eSamplers::Point );
		uint32_t mig19TextureIndex = RegisterBindlessTexture( &bindlessTexturesState, mig19Texture, eSamplers::Point );
		uint32_t bulletTextureIndex = RegisterBindlessTexture( &bindlessTexturesState, bulletTexture, eSamplers::Point );
		uint32_t cloudTextureIndex = RegisterBindlessTexture( &bindlessTexturesState, cloudTexture, eSamplers::Point );
		uint32_t background_sprite_sheet_index = RegisterBindlessTexture( &bindlessTexturesState, background_sprite_sheet, eSamplers::Point );
		uint32_t rescueTextureIndex = RegisterBindlessTexture( &bindlessTexturesState, rescueTexture, eSamplers::Point );

		CreateBackgroundEntity( background_sprite_sheet_index, &gfx_host_visible_mem_allocator );

		gfx_mem_allocator.Commit();

		shipAsset = CreateGfxAsset( quadModel, shipTextureIndex );
		mig19Asset = CreateGfxAsset( quadModel, mig19TextureIndex );
		enemySoldierAsset = CreateGfxAsset( quadModel, rescueTextureIndex );
		bulletRenderable = CreateGfxAsset( quadModel, bulletTextureIndex );
		cloudAsset = CreateGfxAsset( quadModel, cloudTextureIndex );
		rescueAsset = CreateGfxAsset( quadModel, rescueTextureIndex );

		CompileScene( &bindlessTexturesState );

		//Player character
		shipSceneInstance = { glm::vec3( 0.0f, 0.0f, 2.0f ), defaultRotation, shipSize };
		auto hShipTranformationComponent = ecs.CreateComponent<TransformationComponent>( shipSceneInstance );
		auto hShipRenderableComp = ecs.CreateComponent<RenderableComponent>( &shipAsset, false );
		auto hShipUpdateScriptComp = ecs.CreateComponent<ScriptComponent>( UpdateShipScript );
		ecs.CreateEntity( hShipTranformationComponent, hShipRenderableComp, hShipUpdateScriptComp );
		
		cameraSceneInstance = { glm::vec3( 0.0f, 0.0f, -2.0f ), defaultRotation, 1.0f };

		ecs.CreateSingletonComponent<TimeComponent>();
		ecs.CreateSingletonComponent<YKillComponent>( 200.0f );
		ecs.CreateSingletonComponent<DrawlistComponent>();

		auto hEnemyShipSpawnerComponent = ecs.CreateComponent<EnemyShipSpawnerComponent>();
		auto hCreateEnemyShipScript = ecs.CreateComponent<ScriptComponent>( CreateEnemySoldierScript );
		ecs.CreateEntity( hCreateEnemyShipScript, hEnemyShipSpawnerComponent );

		//auto hRescueSpawner = ecs.CreateComponent<EnemyShipSpawnerComponent>();
		//auto hCreateRescueScript = ecs.CreateComponent<ScriptComponent>( CreateRescueScript );
		//ecs.CreateEntity( hCreateRescueScript, hRescueSpawner );

		auto hEnvironmentComponent = ecs.CreateComponent<EnvironmentComponent>();
		auto hCreateCloudsScript = ecs.CreateComponent<ScriptComponent>( CreateCloudScript );
		ecs.CreateEntity( hCreateCloudsScript, hEnvironmentComponent );
	}

	void Destroy() 
	{
		//TODO: "Decompile scene"
		IH::CleanupInputs();
		ConCom::Cleanup();

		AL::Cleanup();

		destroy( &gfx_heap );
		destroy( &gfx_heap_host_visible );
	}
}