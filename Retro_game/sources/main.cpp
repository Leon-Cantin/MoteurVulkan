#include "engine.h"
#include "game.h"

int main() {

	try {
		const Engine::SceneScript gameScene { "Game", Scene3DGame::Init, Scene3DGame::mainLoop, Scene3DGame::cleanup };
		Engine::EngineState engineState( InitRendererImp, CleanupRendererImp, "3D game", Scene3DGame::VIEWPORT_WIDTH, Scene3DGame::VIEWPORT_HEIGHT );

		Engine::RegisterSceneScript( &engineState, gameScene );
		Engine::Start( engineState, "Game" );
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}