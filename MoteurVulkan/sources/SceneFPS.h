#pragma once

#include "vk_globals.h"
#include "vk_image.h"
#include "scene_instance.h"
#include "scene.h"
#include "swapchain.h"
#include "vk_framework.h"
#include "shadow_renderpass.h"
#include "skybox.h"
#include "vk_commands.h"
#include "console_command.h"
#include "input.h"
#include "text_overlay.h"
#include "profile.h"
#include "tick_system.h"
#include "asset_library.h"

#include "camera_orbit.h"
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

namespace SceneFPS
{
	Camera_orbit camera(2);

	uint32_t current_frame = 0;

	bool g_reloadShaders = false;

	const int WIDTH = 800;
	const int HEIGHT = 600;

	SceneInstanceSet g_sceneInstanceDescriptorSets[5];
	size_t g_sceneInstancesCount = 0;

	SceneInstance wandererSceneInstance = { glm::vec3(0.0f, -0.5f, 0.0f), glm::angleAxis(glm::radians(180.0f), glm::vec3{0.0f, 1.0f, 0.0f}), 1.0f };
	RenderableAsset wandererRenderable;

	SceneInstance planeSceneInstance = { glm::vec3(0.0f, -0.5f, 0.0f), glm::angleAxis(glm::radians(0.0f), glm::vec3{0.0f, 1.0f, 0.0f}), 10.0f };
	RenderableAsset planeRenderable;

	SceneInstance cubeSceneInstance = { glm::vec3(0.0f, 0.0f, 2.0f), glm::angleAxis(glm::radians(0.0f), glm::vec3{0.0f, 1.0f, 0.0f}), 0.5f };
	RenderableAsset cubeRenderable;

	SceneInstance cameraSceneInstance = { glm::vec3(0.0f, 0.0f, -2.0f), glm::angleAxis(glm::radians(0.0f), glm::vec3{0.0f, 1.0f, 0.0f}), 1.0f };

	LightUniform g_light{ glm::mat4(1.0f), {3.0f, 3.0f, -3.0f}, 1.0f };

	bool console_active = false;

	float frameDeltaTime = 0.0f;

	extern Swapchain g_swapchain;

	void updateTextOverlayBuffer(uint32_t currentFrame)
	{
		float miliseconds = GetTimestampsDelta(Timestamp::COMMAND_BUFFER_START, Timestamp::COMMAND_BUFFER_END, abs(static_cast<int64_t>(currentFrame) - 1ll));
		char textBuffer[256];
		int charCount = sprintf_s(textBuffer, 256, "GPU: %4.4fms", miliseconds);
		size_t textZonesCount = 1;
		TextZone textZones[2] = { -1.0f, -1.0f, std::string(textBuffer) };
		if (console_active) {
			textZones[1] = { -1.0f, 0.0f, ConCom::GetViewableString() };
			++textZonesCount;
		}
		UpdateText(textZones, textZonesCount, g_swapchain.extent);
	}

	//TODO seperate the buffer update and computation of frame data
	void updateUniformBuffer(uint32_t currentImage) {
		camera.compute_matrix();
		glm::mat4 world_view_matrix = ComputeCameraSceneInstanceViewMatrix(cameraSceneInstance);
		glm::mat4 world_view_matrixx = camera.get_world_view_matrix();

		VkExtent2D swapChainExtent = g_swapchain.extent;

		UpdateGeometryUniformBuffer(&wandererSceneInstance, wandererRenderable.descriptorSet, currentImage);
		UpdateGeometryUniformBuffer(&planeSceneInstance, planeRenderable.descriptorSet, currentImage);
		UpdateGeometryUniformBuffer(&cubeSceneInstance, cubeRenderable.descriptorSet, currentImage);

		UpdateSceneUniformBuffer(world_view_matrix, swapChainExtent, currentImage);

		SceneMatricesUniform shadowSceneMatrices;
		computeShadowMatrix(g_light.position, &shadowSceneMatrices.view, &shadowSceneMatrices.proj);

		UpdateLightUniformBuffer(&shadowSceneMatrices, &g_light, currentImage);

		UpdateSkyboxUniformBuffers(currentImage, world_view_matrix);

		updateTextOverlayBuffer(currentImage);
	}

	void LightCallback(const std::string* params, uint32_t paramsCount)
	{
		if (paramsCount >= 4)
		{
			g_light.position = glm::vec3((float)atof(params[1].data()), (float)atof(params[2].data()), (float)atof(params[3].data()));
		}
	}

	void ConsoleCallback()
	{
		ConCom::ClearConsoleText();
		console_active = true;
	}

	void AcceptCallback()
	{
		if (console_active)
		{
			//TODO: multithreading concerns
			ConCom::SubmitCommand();
			ConCom::ClearConsoleText();
			console_active = false;
		}
	}

	void BackspaceCallback()
	{
		if (console_active)
		{
			ConCom::RemoveConsoleChar();
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
		cameraSceneInstance.location += ForwardVector() * frameDeltaTime;
	}

	void BackwardCallback()
	{
		cameraSceneInstance.location -= ForwardVector() * frameDeltaTime;
	}

	void MoveRightCallback()
	{
		cameraSceneInstance.location += PitchVector() * frameDeltaTime;
	}

	void MoveLeftCallback()
	{
		cameraSceneInstance.location -= PitchVector() * frameDeltaTime;
	}

	void ReloadShadersCallback(const std::string* params, uint32_t paramsCount)
	{
		g_reloadShaders = true;
	}

	bool mouse_pressed = false;
	double cx, cy;
	void glfw_onMouseMove(GLFWwindow* window, double x, double y)
	{
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
	}

	void character_callback(GLFWwindow* window, unsigned int codepoint)
	{
		if (console_active)
		{
			ConCom::CharacterReceived(codepoint);
		}
	}

	void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		if (console_active && (action == GLFW_PRESS || action == GLFW_REPEAT))
		{
			switch (key) {
			case GLFW_KEY_ENTER:
				AcceptCallback();
				break;
			case GLFW_KEY_BACKSPACE:
				BackspaceCallback();
				break;
			case GLFW_KEY_GRAVE_ACCENT:
				ConCom::ClearConsoleText();
				console_active = false;
				break;
			}
		}
		else
		{
			if (action == GLFW_PRESS)
			{
				IH::AddHeldKey(key);
				//CallActionCallbacks(static_cast<uint32_t>(key));
			}
			else if (action == GLFW_RELEASE)
			{
				IH::RemoveHeldKey(key);
			}
		}
	}

	void TickObjectCallback(float dt, void* unused)
	{
		static bool goRight = true;
		cubeSceneInstance.location.x += dt * (goRight ? 0.5f : -0.5f);
		if (abs(cubeSceneInstance.location.x) >= 2.0f)
			goRight ^= true;
	}

	void PrepareSceneFrameData(SceneFrameData* frameData) {
		updateUniformBuffer(current_frame);
		frameData->renderableAssets = { &wandererRenderable, &planeRenderable, &cubeRenderable };
	}

	void mainLoop() {
		while (!glfwWindowShouldClose(g_window))
		{
			static double lastTime = glfwGetTime();
			double currentTime = glfwGetTime();
			frameDeltaTime = float(currentTime - lastTime);
			lastTime = currentTime;

			//Input
			glfwPollEvents();
			IH::DoCommands();

			//Update objects
			TickUpdate(frameDeltaTime);

			//Rendering
			if (g_reloadShaders)
			{
				g_reloadShaders = false;
				ReloadSceneShaders();
			}
			WaitForFrame(current_frame);

			SceneFrameData frameData;
			PrepareSceneFrameData(&frameData);

			draw_frame(current_frame, &frameData);

			current_frame = (++current_frame) % SIMULTANEOUS_FRAMES;
		}

		vkDeviceWaitIdle(g_vk.device);
	}

	void CreateRenderable(const GfxModel* modelAsset, uint32_t albedoIndex, uint32_t normalIndex, SceneRenderableAsset* o_renderable)
	{
		SceneInstanceSet* sceneInstanceSet = &g_sceneInstanceDescriptorSets[g_sceneInstancesCount++];
		CreateGeometryInstanceDescriptorSet(sceneInstanceSet);
		*o_renderable = { modelAsset, sceneInstanceSet, albedoIndex, normalIndex };
	}

	void Init()
	{
		InitRenderer();

		//LoadAssets
		GfxImage* skyboxTexture = AL::LoadCubeTexture("SkyboxTexture", "assets/mountaincube.ktx");

		GfxImage* albedoTexture = AL::CreateSolidColorTexture("ModelAlbedoTexture", glm::vec4(0.8f, 0.8f, 0.8f, 1.0f));
		GfxImage* normalTexture = AL::CreateSolidColorTexture("ModelNormalTexture", glm::vec4(0.0f, 0.0f, 1.0f, 0.0f));

		GfxModel* wandererModelAsset = AL::Load3DModel("Wanderer", "assets/wanderer_decimated.obj", 1);
		GfxModel* planeModelAsset = AL::Load3DModel("Plane", "assets/plane.obj", 0);
		GfxModel* cubeModelAsset = AL::Load3DModel("Cube", "assets/cube.obj", 0);

		CreateDescriptorSets(albedoTexture, normalTexture, skyboxTexture);

		CreateRenderable(wandererModelAsset, 0, 0, &wandererRenderable);
		CreateRenderable(planeModelAsset, 0, 0, &planeRenderable);
		CreateRenderable(cubeModelAsset, 0, 0, &cubeRenderable);

		glfwSetCursorPosCallback(g_window, glfw_onMouseMove);
		glfwSetCharCallback(g_window, character_callback);
		glfwSetKeyCallback(g_window, key_callback);
	}

	void cleanup() {
		CleanupRenderer();

		AL::Cleanup();
	}

	void run() {
		IH::InitInputs();

		ConCom::RegisterCommand("light", &LightCallback);
		ConCom::RegisterCommand("reloadshaders", &ReloadShadersCallback);

		RegisterTickFunction(&TickObjectCallback);

		IH::RegisterAction("console", &ConsoleCallback);
		IH::BindInputToAction("console", GLFW_KEY_GRAVE_ACCENT);
		IH::RegisterAction("forward", &ForwardCallback);
		IH::BindInputToAction("forward", GLFW_KEY_W);
		IH::RegisterAction("backward", &BackwardCallback);
		IH::BindInputToAction("backward", GLFW_KEY_S);
		IH::RegisterAction("left", &MoveLeftCallback);
		IH::BindInputToAction("left", GLFW_KEY_A);
		IH::RegisterAction("right", &MoveRightCallback);
		IH::BindInputToAction("right", GLFW_KEY_D);

		InitFramework(WIDTH, HEIGHT);
		Init();
		mainLoop();
		cleanup();
		ShutdownFramework();
	}
}