#include "engine.h"

const int VIEWPORT_WIDTH = 224;
const int VIEWPORT_HEIGHT = 384;
const int SCREEN_SCALE = 2;
const int SCREEN_WIDTH = VIEWPORT_WIDTH * SCREEN_SCALE;
const int SCREEN_HEIGHT = VIEWPORT_HEIGHT * SCREEN_SCALE;

#include "game_classic_2d.h"
#include "wild_weasel_main_menu.h"

int main() {

	try {
		const Engine::SceneScript gameScene { "Game", WildWeasel_Game::Init, WildWeasel_Game::Update, WildWeasel_Game::Destroy };
		const Engine::SceneScript mainMenuScene { "MainMenu", WildWeasel_Menu::Init, WildWeasel_Menu::Update, WildWeasel_Menu::Destroy };
		Engine::EngineState engineState( InitRendererImp, CleanupRendererImp, "Wild Weasel: Vietnam", SCREEN_WIDTH, SCREEN_HEIGHT );

		Engine::RegisterSceneScript( &engineState, mainMenuScene );
		Engine::RegisterSceneScript( &engineState, gameScene );
		Engine::Start( engineState, "MainMenu" );
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}