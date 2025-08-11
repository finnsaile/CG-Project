#ifndef LEVEL_HPP
#define LEVEL_HPP

#include <iostream>
#include <glm/glm.hpp>
#include <json.hpp>

enum LevelEnum {
    ParkingLot = 0,
    City = 1,
    Showcase = 2
};

struct CarPart {
    std::string name;
    glm::mat4 initial_Wm;
};

struct Pedestal {
    std::string name;
    glm::mat4 initial_Wm;
    std::vector<CarPart> car_body_parts;
    std::vector<CarPart> car_wheels;
};

class Level {
public:
    std::string filename;

    LevelEnum levelId;
    std::string levelName;
    std::string levelScene;
    std::string mainCarPartName;
    std::string checkpointName;
    std::string finalCheckpointName;
    std::vector<std::string> collisionNames;
    std::vector<std::string> carBodyNames;
    std::vector<std::string> carWheelNames;
    std::vector<std::string> carFrontWheelNames;
    std::vector<glm::vec3> checkpoints;
    glm::vec3 startPos;

    std::vector<Pedestal> pedestals;

    float startYaw;
    float startPitch;
    float moveSpeed;
    float moveSpeedBoost;
    float wheelBase;
    float steeringSpeed;
    float maxSteeringAng;
    glm::vec3 camTargetDelta;
    float maxCamDist;
    float minCamDist;
    float defaultCamDist;

    double lowestTime;

    bool initialized = false;
    Level() = default;

    explicit Level(const std::string &filename);

    void updateLowestTime(float newTime);
};

#endif
