#ifndef DRIVINGFRENZY_HPP
#define DRIVINGFRENZY_HPP

#include "modules/GameTimer.hpp"
#include "modules/Starter.hpp"
#include "modules/TextMaker.hpp"
#include "modules/Scene.hpp"
#include "modules/BoundingBox.hpp"
#include "modules/Level.hpp"

enum GameState {
    MenuL1 = 0,
    MenuL2 = 1,
    MenuL3 = 2,
    Playing = 3,
    GameOver = 4,
    Victory = 5,
    Paused = 6,
    Quit = 7,
};

enum HudSize {
    H1920 = 0,
    H1600 = 1,
    H1120 = 2
};

// Used to map between Levels and Menu Game States
const std::unordered_map<GameState, LevelEnum> StateLevelMap{{MenuL1, ParkingLot}, {MenuL2, City}, {MenuL3, Showcase}};
const std::unordered_map<LevelEnum, GameState> LevelStateMap{{ParkingLot, MenuL1}, {City, MenuL2}, {Showcase, MenuL3}};

const std::unordered_map<HudSize, std::pair<unsigned int, unsigned int>> HudSizeMap{{H1920, {1920, 1080}}, {H1600, {1600, 900}}, {H1120, {1120, 630}}};

class DrivingFrenzy : public BaseProject {
protected:
    // Descriptor Layouts [what will be passed to the shaders]
    DescriptorSetLayout DSLglobal, DSLskyBox, DSLphong, DSLoverlay, DSLpbr, DSLghost;

    // Vertex formants, Pipelines [Shader couples] and Render passes
    VertexDescriptor VDphong, VDskyBox, VDoverlay, VDpbr;
    RenderPass RP;
    Pipeline PskyBox, P_Phong, Poverlay, P_PBR, Pghost;

    // Models, textures and Descriptors (values assigned to the uniforms)
    Scene SC;
    std::vector<VertexDescriptorRef> VDRs;
    std::vector<TechniqueRef> PRs;

    // Model and descriptor for the Overlay
    Model MOverlay;
    DescriptorSet DSOverlay;

    // to provide textual feedback
    TextMaker txt;

    // Other application parameters
    float Ar; // Aspect ratio

    glm::mat4 ViewPrj;
    glm::mat4 World;
    glm::vec3 Pos;
    glm::vec3 cameraPos;
    float Yaw = glm::radians(0.0f);
    float Pitch = glm::radians(0.0f);
    float Roll = glm::radians(0.0f);

    // Level and game state
    Level currentLevel;
    LevelEnum currentLevelIdx = ParkingLot;
    GameState currentGameState = MenuL1;
    HudSize currentHudSize = H1600;

    unsigned int currentCheckpointIdx = 0;
    glm::mat4 baseTransformCheckpoint = glm::mat4(1.0f);
    glm::mat4 baseTransformFinalCheckpoint = glm::mat4(1.0f);


    bool sceneChanged = false;

    bool shouldResizeWindow = false;

    bool collisionActive = true;

    bool zoomOut = false;
    float CamDist = 0.f;

    // car model and bounding boxes
    std::unordered_map<std::string, CarPart> carParts;
    std::unordered_map<std::string, BoundingBox> boundingBoxes;

    GameTimer gameTimer;

    float ghostRecordOldTime = 0.0f;
    float ghostRecordAccumulator = 0.0f;

    void mainLoop() override;

    void localInit() override;

    void pipelinesAndDescriptorSetsInit() override;

    void pipelinesAndDescriptorSetsCleanup() override;

    void localCleanup() override;

    static void populateCommandBufferAccess(VkCommandBuffer commandBuffer, int currentImage, void *Params);

    void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage);

    void updateUniformBuffer(uint32_t currentImage) override;

    float GameLogic();

    void setWindowParameters() override;

    void onWindowResize(int w, int h) override;

    void updateScene();

    void loadScene();

    void updateGhostRecording(float gameTime);

    void resetGhostRecordingState();

    void updateGhostPlayback(float gameTime);

    void resetLevel();

    void storeCarParts();

    void storePedestals();

    void storeBoundingBoxes();

    void resizeWindow();

    void setCheckpointPos(unsigned int checkpointIdx);
};

#endif
