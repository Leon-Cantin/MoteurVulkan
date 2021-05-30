#pragma once

#include "retro_renderer_imp.h"
#include "console_command.h"
#include "input.h"
#include "tick_system.h"
#include "asset_library.h"
#include "window_handler_vk.h"
#include "gfx_heaps_batched_allocator.h"
#include "retro_physics.h"
#include "glTF_loader.h"

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

	std::vector<GfxAssetInstance> gfxInstancedAssets;

	SceneInstance shipSceneInstance;
	GfxAsset cubeRenderable;

	SceneInstance cameraSceneInstance;

	BindlessTexturesState bindlessTexturesState;

	LightUniform g_light;

	float frameDeltaTime = 0.0f;

	R_HW::GfxHeap gfx_heap_device_local;

	phs::CollisionMesh groundPlaneCollisionMesh;

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

		const glm::fquat cameraRotationQuat = glm::angleAxis( angle, axis );
		cameraSceneInstance.location = glm::rotate( cameraRotationQuat, cameraSceneInstance.location );
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

	constexpr float movementSpeed = 5.0f;

	void ForwardCallback()
	{
		shipSceneInstance.location += ForwardVector() * (frameDeltaTime / 1000.0f) * movementSpeed;
	}

	void BackwardCallback()
	{
		shipSceneInstance.location -= ForwardVector() * (frameDeltaTime / 1000.0f) * movementSpeed;
	}

	void MoveRightCallback()
	{
		shipSceneInstance.location += PitchVector() * (frameDeltaTime / 1000.0f) * movementSpeed;
	}

	void MoveLeftCallback()
	{
		shipSceneInstance.location -= PitchVector() * (frameDeltaTime / 1000.0f) * movementSpeed;
	}

	void PhsDrawDebugCallback(const std::string* params, uint32_t paramsCount)
	{
		if( paramsCount > 1 )
		{
			bool value = atoi( params[1].c_str() );
			SetBtDebugDraw( value );
		}
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

		phs::Update( frameDeltaTime, &shipSceneInstance );

		std::vector<GfxAssetInstance> drawList = { { &cubeRenderable, shipSceneInstance} };

		drawList.reserve( drawList.size() + gfxInstancedAssets.size() );
		for( const GfxAssetInstance& instanceAsset : gfxInstancedAssets )
			drawList.push_back( instanceAsset );

		DrawFrame( current_frame, &cameraSceneInstance, &g_light, drawList);

		current_frame = (++current_frame) % SIMULTANEOUS_FRAMES;
	}

	void CreateGfxAsset(const GfxModel* modelAsset, uint32_t albedoIndex, GfxAsset* o_renderable)
	{
		*o_renderable = { modelAsset, { albedoIndex } };
	}

	SceneInstance* GetInstancedAssetSlot( const char* name, GfxAsset* asset )
	{
		gfxInstancedAssets.push_back( GfxAssetInstance() );
		gfxInstancedAssets[gfxInstancedAssets.size() - 1].asset = asset;
		return &gfxInstancedAssets[gfxInstancedAssets.size() - 1].instanceData;
	}

	uint32_t LoadTexture( const char* path, I_ImageAlloctor* imageAllocator )
	{
		R_HW::GfxImage* image = AL::LoadTexture( path, path, imageAllocator );
		return RegisterBindlessTexture( &bindlessTexturesState, image );
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
		ConCom::RegisterCommand( "phs_draw_debug", &PhsDrawDebugCallback );

		//Objects update callbacks
		RegisterTickFunction( &TickObjectCallback );

		gfx_heap_device_local = R_HW::create_gfx_heap( 16 * 1024 * 1024, R_HW::GFX_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
		GfxHeaps_BatchedAllocator gfx_device_local_mem_allocator( &gfx_heap_device_local );
		gfx_device_local_mem_allocator.Prepare();

		//LoadAssets
		R_HW::GfxImage* skyboxTexture = AL::LoadCubeTexture( "SkyboxTexture", "assets/mountaincube.ktx", &gfx_device_local_mem_allocator );

		R_HW::GfxImage* albedoTexture = AL::CreateSolidColorTexture( "ModelAlbedoTexture", glm::vec4( 0.8f, 0.8f, 0.8f, 1.0f ), &gfx_device_local_mem_allocator );
		R_HW::GfxImage* BadHelicopterAlbedoTexture = AL::LoadTexture( "BadHelicopterAlbedoTexture", "assets/Tructext.png", &gfx_device_local_mem_allocator );

		const char* groundFileName = "assets/ground.glb";
		GfxModel* cubeModelAsset = AL::LoadglTf3DModel( "Cube", "assets/horrible_helicopter.glb", &gfx_device_local_mem_allocator );

		glTF_L::LoadCollisionData( groundFileName, &groundPlaneCollisionMesh.vertices, &groundPlaneCollisionMesh.indices );

		glTF_L::LoadScene( "assets/scene.gltf", AL::AL_GetModelSlot, AL::AL_GetAssetSlot, GetInstancedAssetSlot, LoadTexture, &gfx_device_local_mem_allocator, &gfx_device_local_mem_allocator );

		uint32_t albedoIndex = RegisterBindlessTexture( &bindlessTexturesState, albedoTexture );
		uint32_t badHelicopterTextIndex = RegisterBindlessTexture( &bindlessTexturesState, BadHelicopterAlbedoTexture );

		gfx_device_local_mem_allocator.Commit();

		CreateGfxAsset( cubeModelAsset, badHelicopterTextIndex, &cubeRenderable );

		CompileScene( &bindlessTexturesState, skyboxTexture );

		shipSceneInstance = { glm::vec3( 0.0f, 1.0f, 2.0f ), glm::angleAxis( glm::radians( 0.0f ), glm::vec3{0.0f, 1.0f, 0.0f} ), 0.5f };
		cameraSceneInstance = { glm::vec3( 0.0f, 1.0f, -6.0f ), glm::angleAxis( glm::radians( 0.0f ), glm::vec3{0.0f, 1.0f, 0.0f} ), 1.0f, &shipSceneInstance };
		g_light = { glm::mat4( 1.0f ), {3.0f, 3.0f, 1.0f}, 1.0f };

		phs::CreateState( btVector3( shipSceneInstance.location.x, shipSceneInstance.location.y, shipSceneInstance.location.z ), 0.5f, groundPlaneCollisionMesh );
	}

	void cleanup() 
	{
		phs::Destroy();
		IH::CleanupInputs();
		ConCom::Cleanup();

		AL::Cleanup();
		destroy( &gfx_heap_device_local );
	}
}