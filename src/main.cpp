#include "modules/DrivingFrenzy.hpp"
// This is the main: probably you do not need to touch this!
int main() {
    DrivingFrenzy app;

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}