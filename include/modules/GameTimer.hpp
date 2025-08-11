#ifndef GAMETIMER_HPP
#define GAMETIMER_HPP

class GameTimer {
    double startTime = 0.0;
    double pausedTime = 0.0;
    double pauseStartTime = 0.0;
    bool isPaused = false;

public:
    bool hasStarted = false;

    void start();

    void pause();

    void resume();

    void reset();

    double getTime() const;
};

#endif
