#pragma once

#include "retro_renderer_imp.h"
#include "console_command.h"
#include "input.h"
#include "tick_system.h"
#include "asset_library.h"
#include "window_handler_vk.h"
#include "gfx_heaps_batched_allocator.h"

#include "btBulletCollisionCommon.h"

#include <glm/glm.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <chrono>
#include <iostream>
#include <vector>
#include <algorithm>
#include <assert.h>

namespace Scene3DGame
{
	uint32_t current_frame = 0;

	const int VIEWPORT_WIDTH = 800;
	const int VIEWPORT_HEIGHT = 600;

	SceneInstance planeSceneInstance;
	GfxAsset planeRenderable;

	SceneInstance shipSceneInstance;
	GfxAsset cubeRenderable;

	SceneInstance cameraSceneInstance;

	BindlessTexturesState bindlessTexturesState;

	LightUniform g_light;

	float frameDeltaTime = 0.0f;

	GfxHeap gfx_heap_device_local;

	btDefaultCollisionConfiguration* collisionConfig;
	btCollisionDispatcher* dispatcher;
	btDbvtBroadphase* overlappingPairCache;
	btCollisionWorld* btWorld;

	btSphereShape mainCharacterCollisionShape( 0.0f );
	btCollisionObject mainCharacterCollisionObject;

	btStaticPlaneShape planeCollisionShape ( btVector3( 0.0f, 1.0f, 0.0f ), 0.0f );
	btCollisionObject planeCollisionObject;

	void LightCallback(const std::string* params, uint32_t paramsCount)
	{
		if (paramsCount >= 4)
		{
			g_light.position = glm::vec3((float)atof(params[1].data()), (float)atof(params[2].data()), (float)atof(params[3].data()));
		}
	}

	glm::vec3 ForwardVector()
	{
		return glm::axis(cameraSceneInstance.orientation * glm::fquat{ 0.0f, 0.0f, 0.0f, 1.0f } *glm::conjugate(cameraSceneInstance.orientation));
	}

	glm::fquat YawQuat()
	{
		return cameraSceneInstance.orientation * glm::fquat { 0.0f, 0.0f, 1.0f, 0.0f } *glm::conjugate( cameraSceneInstance.orientation );
	}

	glm::vec3 YawVector()
	{
		return glm::axis( YawQuat() );
	}

	glm::vec3 PitchVector()
	{
		return glm::axis(cameraSceneInstance.orientation * glm::fquat{ 0.0f, 1.0f, 0.0f, 0.0f } *glm::conjugate(cameraSceneInstance.orientation));
	}

	void CameraYaw( float angle )
	{
		const glm::vec3 axis = { 0.0f, 1.0f, 0.0f };
		cameraSceneInstance.orientation = glm::rotate( cameraSceneInstance.orientation, angle, axis );

		const glm::vec3 shipToCameraVec = cameraSceneInstance.location - shipSceneInstance.location;
		const glm::fquat cameraRotationQuat = glm::angleAxis( angle, axis );
		const glm::vec3 rotatedShipToCameraVec = glm::rotate( cameraRotationQuat, shipToCameraVec );
		cameraSceneInstance.location = shipSceneInstance.location + rotatedShipToCameraVec;
	}

	void CameraYawLeft()
	{
		const float angle = -1.00f * (frameDeltaTime / 1000.0f);
		CameraYaw( angle );
	}

	void CameraYawRight()
	{
		const float angle = 1.00f * (frameDeltaTime / 1000.0f);
		CameraYaw( angle );
	}

	void ForwardCallback()
	{
		cameraSceneInstance.location += ForwardVector() * (frameDeltaTime / 1000.0f);
		shipSceneInstance.location += ForwardVector() * (frameDeltaTime / 1000.0f);
	}

	void BackwardCallback()
	{
		cameraSceneInstance.location -= ForwardVector() * (frameDeltaTime / 1000.0f);
		shipSceneInstance.location -= ForwardVector() * (frameDeltaTime / 1000.0f);
	}

	void MoveRightCallback()
	{
		cameraSceneInstance.location += PitchVector() * (frameDeltaTime / 1000.0f);
		shipSceneInstance.location += PitchVector() * (frameDeltaTime / 1000.0f);
	}

	void MoveLeftCallback()
	{
		cameraSceneInstance.location -= PitchVector() * (frameDeltaTime / 1000.0f);
		shipSceneInstance.location -= PitchVector() * (frameDeltaTime / 1000.0f);
	}

	void ReloadShadersCallback(const std::string* params, uint32_t paramsCount)
	{
		//ForceReloadShaders();
	}

	void TickObjectCallback(float dt, void* unused)
	{
		/*static bool goRight = true;
		shipSceneInstance.location.x += (dt/1000.0f) * (goRight ? 0.5f : -0.5f);
		if (abs(shipSceneInstance.location.x) >= 2.0f)
			goRight ^= true;*/
	}

	void mainLoop() {
		size_t currentTime = WH::GetTime();
		static size_t lastTime = currentTime;
			
		frameDeltaTime = static_cast<float>(currentTime - lastTime);
		lastTime = currentTime;

		//Input
		IH::DoCommands();

		//Update objects
		TickUpdate(frameDeltaTime);

		shipSceneInstance.location.y -= 0.1f * ( frameDeltaTime / 1000.0f );

		mainCharacterCollisionObject.getWorldTransform().setOrigin( btVector3( shipSceneInstance.location.x, shipSceneInstance.location.y, shipSceneInstance.location.z ) );

		btWorld->performDiscreteCollisionDetection();
		const auto numManifolds = btWorld->getDispatcher()->getNumManifolds();
		for( int i = 0; i < numManifolds; i++ ) {
			btPersistentManifold* contactManifold = btWorld->getDispatcher()->getManifoldByIndexInternal( i );
			const auto numContacts = contactManifold->getNumContacts();
			if( numContacts > 0 )
				auto truc = contactManifold->getBody0();
		}

		std::vector<GfxAssetInstance> drawList = { {&planeRenderable, planeSceneInstance}, { &cubeRenderable, shipSceneInstance} };
		DrawFrame( current_frame, &cameraSceneInstance, &g_light, drawList);

		current_frame = (++current_frame) % SIMULTANEOUS_FRAMES;
	}

	void CreateGfxAsset(const GfxModel* modelAsset, uint32_t albedoIndex, GfxAsset* o_renderable)
	{
		*o_renderable = { modelAsset, { albedoIndex } };
	}

	void Init()
	{
		//Input callbacks
		IH::InitInputs();
		IH::RegisterAction( "console", IH::TILD );
		IH::BindAction( "console", IH::Pressed, &ConCom::OpenConsole );
		IH::RegisterAction( "forward", IH::W );
		IH::BindAction( "forward", IH::Pressed, &ForwardCallback );
		IH::RegisterAction( "backward", IH::S );
		IH::BindAction( "backward", IH::Pressed, &BackwardCallback );
		IH::RegisterAction( "left", IH::A );
		IH::BindAction( "left", IH::Pressed, &MoveLeftCallback );
		IH::RegisterAction( "right", IH::D );
		IH::BindAction( "right", IH::Pressed, &MoveRightCallback );
		IH::RegisterAction( "yaw_left", IH::Q );
		IH::BindAction( "yaw_left", IH::Pressed, &CameraYawLeft );
		IH::RegisterAction( "yaw_right", IH::E );
		IH::BindAction( "yaw_right", IH::Pressed, &CameraYawRight );

		//Console commands callback (need IH)
		ConCom::Init();
		ConCom::RegisterCommand( "light", &LightCallback );
		ConCom::RegisterCommand( "reloadshaders", &ReloadShadersCallback );

		//Objects update callbacks
		RegisterTickFunction( &TickObjectCallback );

		gfx_heap_device_local = create_gfx_heap( 16 * 1024 * 1024, GFX_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
		GfxHeaps_BatchedAllocator gfx_device_local_mem_allocator( &gfx_heap_device_local );
		gfx_device_local_mem_allocator.Prepare();

		//LoadAssets
		GfxImage* skyboxTexture = AL::LoadCubeTexture( "SkyboxTexture", "assets/mountaincube.ktx", &gfx_device_local_mem_allocator );

		GfxImage* albedoTexture = AL::CreateSolidColorTexture( "ModelAlbedoTexture", glm::vec4( 0.8f, 0.8f, 0.8f, 1.0f ), &gfx_device_local_mem_allocator );
		GfxImage* BadHelicopterAlbedoTexture = AL::LoadTexture( "BadHelicopterAlbedoTexture", "assets/Tructext.png", &gfx_device_local_mem_allocator );

		GfxModel* planeModelAsset = AL::Load3DModel( "Plane", "assets/plane.obj", 0, &gfx_device_local_mem_allocator );
		GfxModel* cubeModelAsset = AL::LoadglTf3DModel( "Cube", "assets/horrible_helicopter.glb", &gfx_device_local_mem_allocator );

		uint32_t albedoIndex = RegisterBindlessTexture( &bindlessTexturesState, albedoTexture );
		uint32_t badHelicopterTextIndex = RegisterBindlessTexture( &bindlessTexturesState, BadHelicopterAlbedoTexture );

		gfx_device_local_mem_allocator.Commit();

		CreateGfxAsset( planeModelAsset, albedoIndex, &planeRenderable );
		CreateGfxAsset( cubeModelAsset, badHelicopterTextIndex, &cubeRenderable );

		CompileScene( &bindlessTexturesState, skyboxTexture );

		planeSceneInstance = { glm::vec3( 0.0f, 0.0f, 0.0f ), glm::angleAxis( glm::radians( 0.0f ), glm::vec3{0.0f, 1.0f, 0.0f} ), 10.0f };
		shipSceneInstance = { glm::vec3( 0.0f, 1.0f, 2.0f ), glm::angleAxis( glm::radians( 0.0f ), glm::vec3{0.0f, 1.0f, 0.0f} ), 0.5f };
		cameraSceneInstance = { glm::vec3( 0.0f, 1.0f, -4.0f ), glm::angleAxis( glm::radians( 0.0f ), glm::vec3{0.0f, 1.0f, 0.0f} ), 1.0f };
		g_light = { glm::mat4( 1.0f ), {3.0f, 3.0f, 1.0f}, 1.0f };

		collisionConfig = new btDefaultCollisionConfiguration();
		dispatcher = new btCollisionDispatcher( collisionConfig );
		overlappingPairCache = new btDbvtBroadphase();
		btWorld = new btCollisionWorld( dispatcher, overlappingPairCache, collisionConfig );

		mainCharacterCollisionShape = btSphereShape( 0.5f );
		mainCharacterCollisionObject = btCollisionObject();
		mainCharacterCollisionObject.setCollisionShape( &mainCharacterCollisionShape );
		btTransform playerWorld;
		playerWorld.setIdentity();
		playerWorld.setOrigin( btVector3( shipSceneInstance.location.x, shipSceneInstance.location.y, shipSceneInstance.location.z ) );
		mainCharacterCollisionObject.setWorldTransform( playerWorld );
		btWorld->addCollisionObject( &mainCharacterCollisionObject );

		planeCollisionShape = btStaticPlaneShape( btVector3( 0.0f, 1.0f, 0.0f ), 0.0f );
		planeCollisionObject = btCollisionObject();
		planeCollisionObject.setCollisionShape( &planeCollisionShape );
		btTransform planeTransform;
		planeTransform.setIdentity();
		planeTransform.setOrigin( btVector3( planeSceneInstance.location.x, planeSceneInstance.location.y, planeSceneInstance.location.z ) );
		planeCollisionObject.setWorldTransform( planeTransform );
		btWorld->addCollisionObject( &planeCollisionObject );
	}

	void cleanup() 
	{
		IH::CleanupInputs();
		ConCom::Cleanup();

		AL::Cleanup();
		destroy( &gfx_heap_device_local );
	}
}