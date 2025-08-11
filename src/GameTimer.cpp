#include "modules/GameTimer.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>


void GameTimer::start() {
    startTime = glfwGetTime();
    pausedTime = 0.0;
    isPaused = false;
    hasStarted = true;
}

void GameTimer::pause() {
    if (!isPaused) {
        isPaused = true;
        pauseStartTime = glfwGetTime();
    }
}

void GameTimer::resume() {
    if (isPaused) {
        isPaused = false;
        pausedTime += glfwGetTime() - pauseStartTime;
    }
}

void GameTimer::reset() {
    hasStarted = false;
}

double GameTimer::getTime() const {
    double now = glfwGetTime();
    return !hasStarted ? 0.f : (isPaused ? pauseStartTime - startTime - pausedTime
                    : now - startTime - pausedTime);
}
