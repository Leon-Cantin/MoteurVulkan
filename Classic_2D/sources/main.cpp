#include "engine.h"

int main() {

	try {
		Engine::Start( mainMenuScene );
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}