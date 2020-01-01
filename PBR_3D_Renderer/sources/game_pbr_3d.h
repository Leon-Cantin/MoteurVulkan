#pragma once

#include "3d_pbr_renderer_imp.h"
#include "vk_framework.h"
#include "console_command.h"
#include "input.h"
#include "tick_system.h"
#include "asset_library.h"
#include "window_handler_vk.h"

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
	const int HEIGHT = 600;

	SceneInstance planeSceneInstance;
	GfxAsset planeRenderable;

	SceneInstance shipSceneInstance;
	GfxAsset cubeRenderable;

	SceneInstance cameraSceneInstance;

	BindlessTexturesState bindlessTexturesState;

	LightUniform g_light;

	float frameDeltaTime = 0.0f;

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

	glm::vec3 YawVector()
	{
		return glm::axis(cameraSceneInstance.orientation * glm::fquat{ 0.0f, 0.0f, 1.0f, 0.0f } *glm::conjugate(cameraSceneInstance.orientation));
	}

	glm::vec3 PitchVector()
	{
		return glm::axis(cameraSceneInstance.orientation * glm::fquat{ 0.0f, 1.0f, 0.0f, 0.0f } *glm::conjugate(cameraSceneInstance.orientation));
	}

	void ForwardCallback()
	{
		cameraSceneInstance.location += ForwardVector() * (frameDeltaTime/1000.0f);
	}

	void BackwardCallback()
	{
		cameraSceneInstance.location -= ForwardVector() * (frameDeltaTime / 1000.0f);
	}

	void MoveRightCallback()
	{
		cameraSceneInstance.location += PitchVector() * (frameDeltaTime / 1000.0f);
	}

	void MoveLeftCallback()
	{
		cameraSceneInstance.location -= PitchVector() * (frameDeltaTime / 1000.0f);
	}

	void ReloadShadersCallback(const std::string* params, uint32_t paramsCount)
	{
		//ForceReloadShaders();
	}

	void TickObjectCallback(float dt, void* unused)
	{
		static bool goRight = true;
		shipSceneInstance.location.x += (dt/1000.0f) * (goRight ? 0.5f : -0.5f);
		if (abs(shipSceneInstance.location.x) >= 2.0f)
			goRight ^= true;
	}

	void mainLoop() {
		while (!WH::shouldClose())
		{
			//TODO: thread this
			WH::ProcessMessages();
			size_t currentTime = WH::GetTime();
			static size_t lastTime = currentTime;
			
			frameDeltaTime = static_cast<float>(currentTime - lastTime);
			lastTime = currentTime;

			//Input
			IH::DoCommands();

			//Update objects
			TickUpdate(frameDeltaTime);

			std::vector<GfxAssetInstance> drawList = { {&planeRenderable, planeSceneInstance}, { &cubeRenderable, shipSceneInstance} };
			DrawFrame( current_frame, &cameraSceneInstance, &g_light, drawList);

			current_frame = (++current_frame) % SIMULTANEOUS_FRAMES;
		}

		vkDeviceWaitIdle(g_vk.device);
	}

	void CreateGfxAsset(const GfxModel* modelAsset, uint32_t albedoIndex, uint32_t normalIndex, GfxAsset* o_renderable)
	{
		*o_renderable = { modelAsset, { albedoIndex, normalIndex } };
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

		//Console commands callback (need IH)
		ConCom::Init();
		ConCom::RegisterCommand( "light", &LightCallback );
		ConCom::RegisterCommand( "reloadshaders", &ReloadShadersCallback );

		//Objects update callbacks
		RegisterTickFunction( &TickObjectCallback );

		//Init renderer stuff
		InitRendererImp( WH::VK::_windowSurface );

		//LoadAssets
		GfxImage* skyboxTexture = AL::LoadCubeTexture( "SkyboxTexture", "assets/mountaincube.ktx" );

		GfxImage* albedoTexture = AL::CreateSolidColorTexture( "ModelAlbedoTexture", glm::vec4( 0.8f, 0.8f, 0.8f, 1.0f ) );
		GfxImage* normalTexture = AL::CreateSolidColorTexture( "ModelNormalTexture", glm::vec4( 0.0f, 0.0f, 1.0f, 0.0f ) );

		GfxModel* planeModelAsset = AL::Load3DModel( "Plane", "assets/plane.obj", 0 );
		GfxModel* cubeModelAsset = AL::LoadglTf3DModel( "Cube", "assets/cube2.glb" );

		uint32_t albedoIndex = RegisterBindlessTexture( &bindlessTexturesState, albedoTexture, eSamplers::Trilinear );
		uint32_t normalIndex = RegisterBindlessTexture( &bindlessTexturesState, normalTexture, eSamplers::Trilinear );

		CreateGfxAsset( planeModelAsset, albedoIndex, normalIndex, &planeRenderable );
		CreateGfxAsset( cubeModelAsset, albedoIndex, normalIndex, &cubeRenderable );

		CompileScene( &bindlessTexturesState, skyboxTexture );

		planeSceneInstance = { glm::vec3( 0.0f, -0.5f, 0.0f ), glm::angleAxis( glm::radians( 0.0f ), glm::vec3{0.0f, 1.0f, 0.0f} ), 10.0f };
		shipSceneInstance = { glm::vec3( 0.0f, 1.0f, 2.0f ), glm::angleAxis( glm::radians( 0.0f ), glm::vec3{0.0f, 1.0f, 0.0f} ), 0.5f };
		cameraSceneInstance = { glm::vec3( 0.0f, 0.0f, -2.0f ), glm::angleAxis( glm::radians( 0.0f ), glm::vec3{0.0f, 1.0f, 0.0f} ), 1.0f };
		g_light = { glm::mat4( 1.0f ), {3.0f, 3.0f, 1.0f}, 1.0f };
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