#include "game_pbr_3d.h"

int main() {

	try {
		Scene2DGame::run();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}