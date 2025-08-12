#include "modules/Level.hpp"
#include <fstream>


Level::Level(const std::string& filename) : filename(filename) {

    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open level file: " + filename);
    }

    nlohmann::json jsonData;
    file >> jsonData;

    levelId = jsonData.at("levelId").get<LevelEnum>();
    levelName = jsonData.at("levelName").get<std::string>();
    levelScene = jsonData.at("levelScene").get<std::string>();
    mainCarPartName = jsonData.at("mainCarPartName").get<std::string>();
    checkpointName = jsonData.at("checkpointName").get<std::string>();
    finalCheckpointName = jsonData.at("finalCheckpointName").get<std::string>();

    startYaw = glm::radians(jsonData.at("startYaw").get<float>());
    startPitch = glm::radians(jsonData.at("startPitch").get<float>());

    moveSpeed = jsonData.at("moveSpeed").get<float>();
    moveSpeedBoost = jsonData.at("moveSpeedBoost").get<float>();
    wheelBase = jsonData.at("wheelBase").get<float>();
    steeringSpeed = glm::radians(jsonData.at("steeringSpeed").get<float>());
    maxSteeringAng = glm::radians(jsonData.at("maxSteeringAng").get<float>());
    camTargetDelta = glm::vec3(0, jsonData.at("camTargetDeltaY").get<float>(), 0);
    maxCamDist = jsonData.at("maxCamDist").get<float>();
    minCamDist = jsonData.at("minCamDist").get<float>();
    defaultCamDist = jsonData.at("defaultCamDist").get<float>();

    lowestTime = jsonData.at("lowestTime").get<float>();
    // load collision names and add the main car part and checkpoint model names
    collisionNames = jsonData.at("collisionNames").get<std::vector<std::string>>();
    collisionNames.insert(collisionNames.end(), {mainCarPartName, checkpointName, finalCheckpointName});

    carBodyNames = jsonData.at("carBodyNames").get<std::vector<std::string>>();
    carWheelNames = jsonData.at("carWheelNames").get<std::vector<std::string>>();
    carFrontWheelNames = jsonData.at("carFrontWheelNames").get<std::vector<std::string>>();

    auto& posJson = jsonData.at("startPos");
    if (posJson.size() != 3) throw std::runtime_error("Invalid startPos format");
    startPos = glm::vec3(posJson[0].get<float>(), posJson[1].get<float>(), posJson[2].get<float>());
    // load array of checkpoints
    checkpoints.clear();
    for (const auto& cp : jsonData.at("checkpoints")) {
        if (cp.size() != 3) throw std::runtime_error("Invalid checkpoint format");
        glm::vec3 point(cp[0].get<float>(), cp[1].get<float>(), cp[2].get<float>());
        checkpoints.push_back(point);
    }

    if (levelId != Showcase) return;

    pedestals.clear();
    for (const auto& pd : jsonData.at("pedestals")) {
        Pedestal new_pedestal;
        new_pedestal.name = pd.at("ped_id").get<std::string>();
        auto bodyParts = pd.at("body_parts").get<std::vector<std::string>>();

        for (const auto& bp : bodyParts) {
            new_pedestal.car_body_parts.push_back({bp, glm::mat4(1.f)});
        }

        auto wheels = pd.at("wheels").get<std::vector<std::string>>();

        for (const auto& we: wheels) {
            new_pedestal.car_wheels.push_back({we, glm::mat4(1.f)});
        }
        pedestals.push_back(new_pedestal);
    }
}

void Level::updateLowestTime(float newTime) {
    if (levelId == Showcase) return;
    // Read the JSON again
    std::ifstream inputFile(filename);
    if (!inputFile.is_open()) {
        throw std::runtime_error("Could not open level file for reading: " + filename);
    }

    nlohmann::json jsonData;
    inputFile >> jsonData;
    inputFile.close();

    // Update if newTime is better
    float oldTime = jsonData["lowestTime"].get<float>();
    if (newTime < oldTime || oldTime < 0.0f) {
        jsonData["lowestTime"] = newTime;

        // Write back to file
        std::ofstream outputFile(filename);
        if (!outputFile.is_open()) {
            throw std::runtime_error("Could not open level file for writing: " + filename);
        }

        outputFile << std::setw(4) << jsonData << std::endl;
        outputFile.close();

        // Optionally also update your in-memory value
        lowestTime = newTime;
    }
}