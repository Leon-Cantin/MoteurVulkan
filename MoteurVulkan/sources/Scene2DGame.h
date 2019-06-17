#pragma once

#include "scene2D_renderer_imp.h"
#include "vk_framework.h"
#include "console_command.h"
#include "input.h"
#include "tick_system.h"
#include "asset_library.h"
#include "window_handler.h"

#include "shadow_renderpass.h"
#include "text_overlay.h"
#include "skybox.h"

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


extern Swapchain g_swapchain;

namespace Scene2DGame
{
	uint32_t current_frame = 0;

	const int WIDTH = 800;
	const int HEIGHT = 600;

	SceneInstance planeSceneInstance;
	SceneRenderableAsset planeRenderable;

	SceneInstance cubeSceneInstance;
	SceneRenderableAsset cubeRenderable;

	SceneInstance cameraSceneInstance;

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
	
	//TODO
	/*static void onMouseMove( double x, double y )
	{
		static bool mouse_pressed = false;
		static double cx, cy;
		if (glfwGetMouseButton(g_window, GLFW_MOUSE_BUTTON_1) == GLFW_PRESS && !mouse_pressed) {
			glfwSetInputMode(g_window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
			glfwGetCursorPos(g_window, &cx, &cy);
			//glfwSetCursorPos(window, cx, cy);
			mouse_pressed = true;
		}
		else if (glfwGetMouseButton(g_window, GLFW_MOUSE_BUTTON_1) == GLFW_RELEASE && mouse_pressed) {
			glfwSetInputMode(g_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			mouse_pressed = false;
		}

		if (mouse_pressed)
		{
			glfwSetCursorPos(g_window, cx, cy);
			float dx = x - cx;
			float dy = y - cy;

			//TODO: Create offsets instead of applying transformation right away
			glm::fquat pitchRotation = glm::angleAxis(glm::radians(dy) * frameDeltaTime, PitchVector());
			glm::fquat yawRotation = glm::angleAxis(glm::radians(dx) * frameDeltaTime, glm::vec3{ 0.0f,1.0f,0.0f });
			cameraSceneInstance.orientation = yawRotation * pitchRotation * cameraSceneInstance.orientation;
		}
	}*/

	void ReloadShadersCallback(const std::string* params, uint32_t paramsCount)
	{
		ForceReloadShaders;
	}

	void TickObjectCallback(float dt, void* unused)
	{
		static bool goRight = true;
		cubeSceneInstance.location.x += (dt/1000.0f) * (goRight ? 0.5f : -0.5f);
		if (abs(cubeSceneInstance.location.x) >= 2.0f)
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

			std::vector<std::pair<const SceneInstance*, const SceneRenderableAsset*>> drawList = { {&planeSceneInstance, &planeRenderable}, { &cubeSceneInstance, &cubeRenderable} };
			DrawFrame( current_frame, &cameraSceneInstance, &g_light, drawList);

			current_frame = (++current_frame) % SIMULTANEOUS_FRAMES;
		}

		vkDeviceWaitIdle(g_vk.device);
	}

	void CreateRenderable(const GfxModel* modelAsset, uint32_t albedoIndex, uint32_t normalIndex, SceneRenderableAsset* o_renderable)
	{
		SceneInstanceSet* sceneInstanceSet = CreateGeometryInstanceDescriptorSet();
		*o_renderable = { modelAsset, sceneInstanceSet, albedoIndex, normalIndex };
	}

	void Init()
	{
		InitFramework(WIDTH, HEIGHT, "2D game");

		//Input callbacks
		IH::InitInputs();
		IH::RegisterAction("console", IH::Pressed, &ConCom::OpenConsole);
		IH::BindInputToAction("console", IH::TILD);
		IH::RegisterAction("forward", IH::Pressed, &ForwardCallback);
		IH::BindInputToAction("forward", IH::W);
		IH::RegisterAction("backward", IH::Pressed, &BackwardCallback);
		IH::BindInputToAction("backward", IH::S);
		IH::RegisterAction("left", IH::Pressed, &MoveLeftCallback);
		IH::BindInputToAction("left", IH::A);
		IH::RegisterAction("right", IH::Pressed, &MoveRightCallback);
		IH::BindInputToAction("right", IH::D);

		//TODO
		//glfwSetCursorPosCallback(g_window, onMouseMove);

		//Console commands callback (need IH)
		ConCom::Init();
		ConCom::RegisterCommand("light", &LightCallback);
		ConCom::RegisterCommand("reloadshaders", &ReloadShadersCallback);

		//Objects update callbacks
		RegisterTickFunction(&TickObjectCallback);

		//Init renderer stuff
		InitRendererImp();

		//LoadAssets
		GfxImage* skyboxTexture = AL::LoadCubeTexture("SkyboxTexture", "assets/mountaincube.ktx");

		GfxImage* albedoTexture = AL::CreateSolidColorTexture("ModelAlbedoTexture", glm::vec4(0.8f, 0.8f, 0.8f, 1.0f));
		GfxImage* normalTexture = AL::CreateSolidColorTexture("ModelNormalTexture", glm::vec4(0.0f, 0.0f, 1.0f, 0.0f));

		GfxModel* planeModelAsset = AL::Load3DModel("Plane", "assets/plane.obj", 0);
		GfxModel* cubeModelAsset = AL::LoadglTf3DModel( "Cube", "assets/cube2.glb" );

		InitSkybox(skyboxTexture);

		CreateGeometryRenderpassDescriptorSet(albedoTexture, normalTexture);

		planeSceneInstance = { glm::vec3(0.0f, -0.5f, 0.0f), glm::angleAxis(glm::radians(0.0f), glm::vec3{0.0f, 1.0f, 0.0f}), 10.0f };
		cubeSceneInstance = { glm::vec3(0.0f, 0.0f, 2.0f), glm::angleAxis(glm::radians(0.0f), glm::vec3{0.0f, 1.0f, 0.0f}), 0.5f };
		cameraSceneInstance = { glm::vec3(0.0f, 0.0f, -2.0f), glm::angleAxis(glm::radians(0.0f), glm::vec3{0.0f, 1.0f, 0.0f}), 1.0f };
		g_light = { glm::mat4(1.0f), {3.0f, 3.0f, -3.0f}, 1.0f };

		CreateRenderable(planeModelAsset, 0, 0, &planeRenderable);
		CreateRenderable(cubeModelAsset, 0, 0, &cubeRenderable);
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
		ShutdownFramework();
	}
}