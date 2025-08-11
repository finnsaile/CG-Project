#include "modules/DrivingFrenzy.hpp"
#include <sstream>

#include <json.hpp>

#include "modules/WVP.hpp"
#include "modules/Animations.hpp"
//
// box xable reel pipe wheelbarrow
// transport_truck_003_transport_truck_003.001
// transport_truck_011_transport_truck_011.001
// Vertex layouts
struct Vertex {
    glm::vec3 pos;
    glm::vec3 norm;
    glm::vec2 UV;
};

struct VertexSkyBox {
    glm::vec3 pos;
};

struct VertexOverlay {
    glm::vec2 pos;
    glm::vec2 UV;
};

struct VertexPbr {
    glm::vec3 pos;
    glm::vec3 norm;
    glm::vec2 UV;
    glm::vec4 tan;
};

// define Uniform Buffers
struct GlobalUniformBufferObject {
    alignas(16) glm::vec3 lightDir;
    alignas(16) glm::vec4 lightColor;
    alignas(16) glm::vec3 eyePos;
};

struct UniformBufferObject {
    alignas(16) glm::mat4 mvpMat;
    alignas(16) glm::mat4 mMat;
    alignas(16) glm::mat4 nMat;
};

struct SkyBoxUniformBufferObject {
    alignas(16) glm::mat4 mvpMat;
};

struct OverlayUniformBuffer {
    alignas(4) float visible;
    alignas(4) int hudVariant;
    alignas(4) int hudSizeVariant;
};

#include <iostream>
#include <cmath>
#include <string>
#include <random>
#include <nlohmann/json.hpp> // for JSON output

static std::map<std::pair<int, int>, int> tileCounters;

static float snapYaw(float yawDeg) {
    // Wrap to [0, 360)
    float y = std::fmod(yawDeg, 360.0f);
    if (y < 0.0f) y += 360.0f;

    // Snap to 0, 90, 180, 270
    float snapped = std::round(y / 90.0f) * 90.0f;
    if (snapped >= 360.0f) snapped = 0.0f; // handle 359.9 -> 360

    // Return in your desired set {0, 90, 180, -90}
    if (snapped == 270.0f) return -90.0f;
    return snapped; // 0, 90, or 180
}

void printTileJsonFromCar(const glm::vec3 &Pos, float Yaw) {
    // --- Determine tile coordinates ---
    int tileX = static_cast<int>(std::floor((Pos.x + 10.0f) / 20.0f));
    int tileZ = static_cast<int>(std::floor((Pos.z + 10.0f) / 20.0f));

    // --- Update per-tile counter ---
    auto tileKey = std::make_pair(tileX, tileZ);
    int &counter = tileCounters[tileKey];
    counter++; // start from 1

    // --- Snap yaw ---
    float yawDeg = glm::degrees(Yaw);
    float snappedYaw = snapYaw(yawDeg);

    // --- Random number for model ---
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dist(1, 15);
    int randomNum = dist(gen);

    // --- Build id with counter ---
    std::string id = "v" + std::to_string(tileX) + "x" + std::to_string(tileZ) + "_" + std::to_string(counter);

    // --- Model with random number ---
    std::string modelName = "carBasicBlack";

    // --- Print exactly formatted output ---
    std::cout << "{\"id\": \"" << id << "\",  "
            << "\"model\": \"" << modelName << "\",   "
            << "\"texture\": [\"ColorAtlasCG\"],\n"
            << "\t\"translate\": ["
            << Pos.x << ", " << -0.15 << ", " << Pos.z << "],\n"
            << "\t\"eulerAngles\": [0, " << snappedYaw << ", 0],\n"
            << "\t\"scale\": [1.25, 1.25, 1.25]\n"
            << "},"
            << std::endl;
}

inline glm::mat4 rotateAroundPivot(glm::vec3 pivotWorld,
                               float radians, glm::vec3 axisWorld)
{
    return glm::translate(glm::mat4(1), pivotWorld) *
           glm::rotate(glm::mat4(1), radians, glm::normalize(axisWorld)) *
           glm::translate(glm::mat4(1), -pivotWorld);
}

void DrivingFrenzy::setWindowParameters() {
    // window size, titile and initial background
    auto [width, height] = HudSizeMap.at(currentHudSize);
    windowWidth = width;
    windowHeight = height;
    windowTitle = "DrivingFrenzy - Vehicle Simulation";
    windowResizable = GLFW_FALSE;

    // Initial aspect ratio
    Ar = 16.0f / 9.0f;
}

// What to do when the window changes size
void DrivingFrenzy::onWindowResize(int w, int h) {
    Ar = (float) w / (float) h;
    // Update Render Pass
    RP.width = w;
    RP.height = h;

    // updates the textual output
    txt.resizeScreen(w, h);
}

void DrivingFrenzy::resizeWindow() {
    GLFWmonitor *primary = glfwGetPrimaryMonitor();

    int monitorX, monitorY, monitorWidth, monitorHeight;
    glfwGetMonitorWorkarea(primary, &monitorX, &monitorY, &monitorWidth, &monitorHeight);
    glfwSetWindowSize(window, windowWidth, windowHeight);
    // Center the window
    int posX = monitorX + (monitorWidth - windowWidth) / 2;
    int posY = monitorY + (monitorHeight - windowHeight) / 2;
    glfwSetWindowPos(window, posX, posY);
    recreateSwapChain();
}

void DrivingFrenzy::loadScene() {
    std::string file;
    switch (currentLevelIdx) {
        case ParkingLot: file = "assets/models/levels/parking_lot_level.json";
            break;
        case City: file = "assets/models/levels/city_level.json";
            break;
        case Showcase: file = "assets/models/levels/showcase_level.json";
            break;
        default: break;
    }

    currentLevel = Level(file);

    // new scene object to delete uncleared maps
    SC = Scene{};
    if (SC.init(this, /*Npasses*/1, VDRs, PRs, currentLevel.levelScene) != 0) {
        std::cout << "ERROR LOADING THE SCENE\n";
        exit(0);
    }

    storeCarParts();
    storeBoundingBoxes();
    storePedestals();

    const int checkpointId = SC.InstanceIds.at(currentLevel.checkpointName);
    const int finalCheckpointId = SC.InstanceIds.at(currentLevel.finalCheckpointName);

    baseTransformCheckpoint = SC.I[checkpointId]->Wm;
    baseTransformFinalCheckpoint = SC.I[finalCheckpointId]->Wm;

    resetLevel();
}

void DrivingFrenzy::resetLevel() {
    Pos = currentLevel.startPos;
    Yaw = currentLevel.startYaw;

    zoomOut = false;

    currentLevel.initialized = false;

    const int checkpointId = SC.InstanceIds.at(currentLevel.checkpointName);
    const int finalCheckpointId = SC.InstanceIds.at(currentLevel.finalCheckpointName);

    SC.I[checkpointId]->Wm = baseTransformCheckpoint;
    SC.I[finalCheckpointId]->Wm = baseTransformFinalCheckpoint;

    currentCheckpointIdx = 0;
    setCheckpointPos(currentCheckpointIdx);
}

void DrivingFrenzy::storeCarParts() {
    // store car body and wheel Wms in map for later access and movement
    carParts.clear();
    for (const auto &name: currentLevel.carBodyNames) {
        if (SC.InstanceIds.count(name) > 0) {
            int id = SC.InstanceIds[name];
            carParts[name] = CarPart{name, SC.I[id]->Wm};
        }
    }

    for (const auto &name: currentLevel.carWheelNames) {
        if (SC.InstanceIds.count(name) > 0) {
            int id = SC.InstanceIds[name];
            carParts[name] = CarPart{name, SC.I[id]->Wm};
        }
    }
}

void DrivingFrenzy::storePedestals() {
    // store car body and wheel Wms in map for later access and movement
    auto& pedestals = currentLevel.pedestals;
    int id = 0;

    for (auto &pd: pedestals) {
        id = SC.InstanceIds[pd.name];
        pd.initial_Wm = SC.I[id]->Wm;

        for (auto &car_part: pd.car_body_parts) {
            id = SC.InstanceIds[car_part.name];
            car_part.initial_Wm = SC.I[id]->Wm;
        }

        for (auto &wheel: pd.car_wheels) {
            id = SC.InstanceIds[wheel.name];
            wheel.initial_Wm = SC.I[id]->Wm;
        }
    }


}

void DrivingFrenzy::storeBoundingBoxes() {
    // calculate and store bounding boxes for all instances that partake in colision
    boundingBoxes.clear();
    for (auto &name: currentLevel.collisionNames) {
        const int instanceId = SC.InstanceIds[name];
        const int modelId = SC.I[instanceId]->Mid;
        const glm::mat4 &Wm = SC.I[instanceId]->Wm;
        const std::vector<unsigned char> &vertices = SC.M[modelId]->vertices;

        boundingBoxes[name] = BoundingBox(name, Wm, vertices);
    }
}

// update the world matrix and bounding box of the checkpoint using the current checkpoint idx
void DrivingFrenzy::setCheckpointPos(const unsigned int checkpointIdx) {
    if (checkpointIdx >= currentLevel.checkpoints.size()) {
        gameTimer.pause();
        currentLevel.updateLowestTime(gameTimer.getTime());
        currentGameState = Victory;
        currentCheckpointIdx = 0;
        return;
    }

    const int checkpointId = SC.InstanceIds.at(currentLevel.checkpointName);
    const int finalCheckpointId = SC.InstanceIds.at(currentLevel.finalCheckpointName);

    glm::mat4 &checkpointWm = baseTransformCheckpoint;
    glm::mat4 &finalCheckpointWm = baseTransformFinalCheckpoint;

    if (checkpointIdx == currentLevel.checkpoints.size() - 1) {
        finalCheckpointWm[3] = glm::vec4(currentLevel.checkpoints[checkpointIdx], 1.0f);
        checkpointWm[3] = glm::vec4(0.f, -100.f, 0.f, 1.f);
        // baseTransformCurrentCheckpoint = finalCheckpointWm;
    } else {
        checkpointWm[3] = glm::vec4(currentLevel.checkpoints[checkpointIdx], 1.0f);
        finalCheckpointWm[3] = glm::vec4(0.f, -100.f, 0.f, 1.f);
        // baseTransformCurrentCheckpoint = checkpointWm;
    }

    boundingBoxes[currentLevel.checkpointName].updateFromWorldMatrix(checkpointWm);
    boundingBoxes[currentLevel.finalCheckpointName].updateFromWorldMatrix(finalCheckpointWm);
}

// updated main loop to accommodate for scene changing OUTSIDE of draw frame
void DrivingFrenzy::mainLoop() {
    // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        if (sceneChanged) {
            updateScene();
            sceneChanged = false;
        }
        if (shouldResizeWindow) {
            resizeWindow();
            shouldResizeWindow = false;
        }
        drawFrame();
    }

    vkDeviceWaitIdle(device);
}

// Here you load and setup all your Vulkan Models and Texutures.
// Here you also create your Descriptor set layouts and load the shaders for the pipelines
void DrivingFrenzy::localInit() {
    // Descriptor Layouts [what will be passed to the shaders]
    DSLglobal.init(this, {
                       {
                           0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS,
                           sizeof(GlobalUniformBufferObject), 1
                       }
                   });

    DSLphong.init(this, {
                      {
                          0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT,
                          sizeof(UniformBufferObject), 1
                      },
                      {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1}
                  });

    DSLskyBox.init(this, {
                       {
                           0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT,
                           sizeof(SkyBoxUniformBufferObject), 1
                       },
                       {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1}
                   });

    DSLoverlay.init(this, {
                        {
                            0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS,
                            sizeof(OverlayUniformBuffer), 1
                        },
                        {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1},
                        {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1},
                        {3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2, 1}
                    });

    DSLoverlay.init(this, {
                        {
                            0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS,
                            sizeof(OverlayUniformBuffer), 1
                        },
                        {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1},
                        {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1},
                        {3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2, 1}
                    });

    DSLpbr.init(this, {
                    {
                        0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                        VK_SHADER_STAGE_VERTEX_BIT,
                        sizeof(UniformBufferObject), 1
                    },
                    {
                        1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                        VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1
                    },
                    {
                        2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                        VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1
                    },
                    {
                        3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                        VK_SHADER_STAGE_FRAGMENT_BIT, 2, 1
                    },
                    {
                        4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                        VK_SHADER_STAGE_FRAGMENT_BIT, 3, 1
                    }
                });


    VDphong.init(this, {
                     {0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX}
                 }, {
                     {
                         0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos),
                         sizeof(glm::vec3), POSITION
                     },
                     {
                         0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, norm),
                         sizeof(glm::vec3), NORMAL
                     },
                     {
                         0, 2, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, UV),
                         sizeof(glm::vec2), UV
                     }
                 });

    VDskyBox.init(this, {
                      {0, sizeof(VertexSkyBox), VK_VERTEX_INPUT_RATE_VERTEX}
                  }, {
                      {
                          0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexSkyBox, pos),
                          sizeof(glm::vec3), POSITION
                      }
                  });

    VDoverlay.init(this, {
                       {0, sizeof(VertexOverlay), VK_VERTEX_INPUT_RATE_VERTEX}
                   }, {
                       {
                           0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(VertexOverlay, pos),
                           sizeof(glm::vec2), OTHER
                       },
                       {
                           0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(VertexOverlay, UV),
                           sizeof(glm::vec2), UV
                       }
                   });

    VDpbr.init(this, {
                   // one binding, per-vertex
                   {0, sizeof(VertexPbr), VK_VERTEX_INPUT_RATE_VERTEX}
               }, {
                   {
                       0, 0, VK_FORMAT_R32G32B32_SFLOAT,
                       offsetof(VertexPbr, pos),
                       sizeof(glm::vec3), POSITION
                   },
                   {
                       0, 1, VK_FORMAT_R32G32B32_SFLOAT,
                       offsetof(VertexPbr, norm),
                       sizeof(glm::vec3), NORMAL
                   },
                   {
                       0, 2, VK_FORMAT_R32G32_SFLOAT,
                       offsetof(VertexPbr, UV),
                       sizeof(glm::vec2), UV
                   },
                   {
                       0, 3, VK_FORMAT_R32G32B32A32_SFLOAT,
                       offsetof(VertexPbr, tan),
                       sizeof(glm::vec4), TANGENT
                   }
               });


    VDRs.resize(3);

    VDRs[0].init("VDphong", &VDphong);
    VDRs[1].init("VDskybox", &VDskyBox);
    VDRs[2].init("VDpbr", &VDpbr);

    // initializes the render passes
    RP.init(this);
    // sets the blue sky
    RP.properties[0].clearValue = {0.0f, 0.9f, 1.0f, 1.0f};


    // Pipelines [Shader couples]
    // The last array, is a vector of pointer to the layouts of the sets that will
    // be used in this pipeline. The first element will be set 0, and so on..
    P_Phong.init(this, &VDphong, "shaders/PhongShader.vert.spv", "shaders/PhongShader.frag.spv",
                 {&DSLglobal, &DSLphong});
    P_Phong.setCompareOp(VK_COMPARE_OP_LESS_OR_EQUAL);
    P_Phong.setCullMode(VK_CULL_MODE_NONE);
    P_Phong.setPolygonMode(VK_POLYGON_MODE_FILL);
    P_Phong.setTransparency(false);

    PskyBox.init(this, &VDskyBox, "shaders/SkyBoxShader.vert.spv", "shaders/SkyBoxShader.frag.spv", {&DSLskyBox});
    PskyBox.setCompareOp(VK_COMPARE_OP_LESS_OR_EQUAL);
    PskyBox.setCullMode(VK_CULL_MODE_BACK_BIT);
    PskyBox.setPolygonMode(VK_POLYGON_MODE_FILL);

    Poverlay.init(this, &VDoverlay, "shaders/Overlay.vert.spv", "shaders/Overlay.frag.spv", {&DSLoverlay});
    Poverlay.setCompareOp(VK_COMPARE_OP_LESS_OR_EQUAL);
    Poverlay.setCullMode(VK_CULL_MODE_NONE);
    Poverlay.setPolygonMode(VK_POLYGON_MODE_FILL);
    Poverlay.setTransparency(true);

    P_PBR.init(this,&VDpbr, "shaders/PBR.vert.spv", "shaders/PBR.frag.spv", {&DSLglobal, &DSLpbr});
    P_PBR.setCompareOp(VK_COMPARE_OP_LESS_OR_EQUAL);
    P_PBR.setCullMode(VK_CULL_MODE_BACK_BIT);
    P_PBR.setPolygonMode(VK_POLYGON_MODE_FILL);
    P_PBR.setTransparency(false);


    PRs.resize(3);

    PRs[0].init("PhongShader", {
                    {
                        &P_Phong, {
                            {}, // DSLglobal
                            {
                                {true, 0, {}}, // DSLphong with one texture
                            }
                        }
                    }
                }, 1, &VDphong);
    PRs[1].init("SkyBox", {
                    {
                        &PskyBox, {
                            {
                                {true, 0, {}} // DSLskyBox with one texture
                            }
                        }
                    }
                }, 1, &VDskyBox);

    PRs[2].init("PBR", {
    {
        &P_PBR,
        {
            {}, // set 0 (DSLglobal) – nothing to pre-bind here
            // set 1 (DSLpbr): bind 5 samplers — albedo, normal, metallic, roughness, ao
            {
                { true, 0, {} }, // albedoMap    at binding 1 (array idx empty -> first/only)
                { true, 1, {} }, // normalMap    at binding 2
                { true, 2, {} }, // metallicMap  at binding 3
                { true, 3, {} }, // roughnessMap at binding 4
            }
        }
    }
}, 4, &VDpbr);

    MOverlay.vertices = std::vector<unsigned char>(4 * sizeof(VertexOverlay));
    VertexOverlay *V1 = (VertexOverlay *) (&(MOverlay.vertices[0]));

    V1[0] = {{-1.f, 1.f}, {0.0f, 1.0f}}; // top-left
    V1[1] = {{-1.f, -1.f}, {0.0f, 0.0f}}; // bottom-left
    V1[2] = {{1.f, 1.f}, {1.0f, 1.0f}}; // top-right
    V1[3] = {{1.f, -1.f}, {1.0f, 0.0f}}; // bottom-right
    MOverlay.indices = {0, 1, 2, 2, 1, 3};
    MOverlay.initMesh(this, &VDoverlay);


    // sets the size of the Descriptor Set Pool
    DPSZs.uniformBlocksInPool = 6;
    DPSZs.texturesInPool = 7;
    DPSZs.setsInPool = 6;

    loadScene();

    static bool init = true;
    // initializes the textual output
    if (init) {
        txt.init(this, windowWidth, windowHeight);
        init = false;
    }

    // submits the main command buffer
    submitCommandBuffer("main", 0, populateCommandBufferAccess, this);

    // Prepares for showing the FPS count
    // txt.print(1.0f, 1.0f, "FPS:", 1, "CO", false, false, true, TAL_RIGHT, TRH_RIGHT, TRV_BOTTOM,
    //           {1.0f, 1.0f, 1.0f, 1.0f}, {0.8f, 0.8f, 0.0f, 1.0f});
}

// Here you create your pipelines and Descriptor Sets!
void DrivingFrenzy::pipelinesAndDescriptorSetsInit() {
    // creates the render pass
    RP.create();

    // This creates a new pipeline (with the current surface), using its shaders for the provided render pass
    P_Phong.create(&RP);
    PskyBox.create(&RP);
    Poverlay.create(&RP);
    P_PBR.create(&RP);

    const int hudId_1920 = SC.TextureIds["hud_1920"];
    const int hudId_1600 = SC.TextureIds["hud_1600"];
    const int hudId_1120 = SC.TextureIds["hud_1120"];
    DSOverlay.init(this, &DSLoverlay, {
                       SC.T[hudId_1920]->getViewAndSampler(), SC.T[hudId_1600]->getViewAndSampler(),
                       SC.T[hudId_1120]->getViewAndSampler()
                   });

    SC.pipelinesAndDescriptorSetsInit();
    txt.pipelinesAndDescriptorSetsInit();
}

// Here you destroy your pipelines and Descriptor Sets!
void DrivingFrenzy::pipelinesAndDescriptorSetsCleanup() {
    P_Phong.cleanup();
    PskyBox.cleanup();
    Poverlay.cleanup();
    P_PBR.cleanup();
    RP.cleanup();

    DSOverlay.cleanup();

    SC.pipelinesAndDescriptorSetsCleanup();
    txt.pipelinesAndDescriptorSetsCleanup();
}

// Here you destroy all the Models, Texture and Desc. Set Layouts you created!
// You also have to destroy the pipelines
void DrivingFrenzy::localCleanup() {
    MOverlay.cleanup();

    DSLskyBox.cleanup();
    DSLglobal.cleanup();
    DSLphong.cleanup();
    DSLoverlay.cleanup();
    DSLpbr.cleanup();

    PskyBox.destroy();
    P_Phong.destroy();
    Poverlay.destroy();
    P_PBR.destroy();

    RP.destroy();

    SC.localCleanup();

    if (!sceneChanged)
        txt.localCleanup();

    // for(int ian = 0; ian < N_ANIMATIONS; ian++) {
    // 	Anim[ian].cleanup();
    // }
}

// Here it is the creation of the command buffer:
// You send to the GPU all the objects you want to draw,
// with their buffers and textures
void DrivingFrenzy::populateCommandBufferAccess(VkCommandBuffer commandBuffer, int currentImage, void *Params) {
    // Simple trick to avoid having always 'T->'
    // in che code that populates the command buffer!
    DrivingFrenzy *T = (DrivingFrenzy *) Params;
    T->populateCommandBuffer(commandBuffer, currentImage);
}

// This is the real place where the Command Buffer is written
void DrivingFrenzy::populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage) {
    // begin standard pass
    RP.begin(commandBuffer, currentImage);

    SC.populateCommandBuffer(commandBuffer, 0, currentImage);

    Poverlay.bind(commandBuffer);
    MOverlay.bind(commandBuffer);
    DSOverlay.bind(commandBuffer, Poverlay, 0, currentImage);
    vkCmdDrawIndexed(commandBuffer,
                     static_cast<uint32_t>(MOverlay.indices.size()), 1, 0, 0, 0);

    RP.end(commandBuffer);
}

void DrivingFrenzy::updateScene() {
    vkDeviceWaitIdle(device);

    cleanupSwapChain();

    localCleanup();

    createSwapChain(); // Recreate swapchain and get new swapChainImages
    createImageViews(); // Image views from the new images

    localInit(); // Init scene-specific stuff with new swapchain
    createDescriptorPool();
    pipelinesAndDescriptorSetsInit();
}

// Here is where you update the uniforms.
// Very likely this will be where you will be writing the logic of your application.
void DrivingFrenzy::updateUniformBuffer(uint32_t currentImage) {
    static bool debounce = false;
    static int curDebounce = 0;

    // lamda function handling debounce logic,
    // takes a key and a function reference to execute when key is pressed
    auto handleKey = [&](int key, auto &&action) {
        if (glfwGetKey(window, key)) {
            if (!debounce) {
                debounce = true;
                curDebounce = key;
                action();
            }
        } else if (debounce && curDebounce == key) {
            debounce = false;
            curDebounce = 0;
        }
    };


    const std::unordered_map<int, GameState> keyToState = {
        {GLFW_KEY_1, MenuL1},
        {GLFW_KEY_2, MenuL2},
        {GLFW_KEY_3, MenuL3},
    };

    // logic for all menu keys is the same:
    // check if game state has changed and if so, update the level
    for (const auto &[key, targetState]: keyToState) {
        handleKey(key, [&]() {
            if ((currentGameState == MenuL1 || currentGameState == MenuL2 || currentGameState == MenuL3) &&
                currentGameState != targetState) {
                currentGameState = targetState;
                currentLevelIdx = StateLevelMap.at(currentGameState);
                sceneChanged = true;
            }
        });
    }

    // handle space key, no matter what state, space changes state to Playing
    // additionally reset level on GameOver or Victory
    handleKey(GLFW_KEY_SPACE, [&]() {
        if (currentGameState == GameOver || currentGameState == Victory) {
            resetLevel();
            gameTimer.start();
        } else if (currentGameState == MenuL1 || currentGameState == MenuL2 || currentGameState == MenuL3) {
            gameTimer.start();
        } else if (currentGameState == Paused) {
            gameTimer.resume();
        }

        currentGameState = Playing;
    });

    // handle escape key
    handleKey(GLFW_KEY_ESCAPE, [&]() {
        // if back to menu, reset level and set menu state according to current level
        if (currentGameState == GameOver || currentGameState == Victory || currentGameState == Paused) {
            resetLevel();
            currentGameState = LevelStateMap.at(currentLevelIdx);
            gameTimer.reset();
            // close app when in menu
        } else if (currentGameState == MenuL1 || currentGameState == MenuL2 || currentGameState == MenuL3) {
            glfwSetWindowShouldClose(window, GL_TRUE);
            // pause if playing
        } else if (currentGameState == Playing) {
            currentGameState = Paused;
            gameTimer.pause();
        }
    });

    handleKey(GLFW_KEY_C, [&]() {
        // if back to menu, reset level and set menu state according to current level
        if (currentGameState == Playing) {
            std::cout << "[" << Pos.x << ", " << Pos.y << ", " << Pos.z << "]," << std::endl;
        }
    });

    handleKey(GLFW_KEY_L, [&]() {
        // if back to menu, reset level and set menu state according to current level
        collisionActive = !collisionActive;
    });

    handleKey(GLFW_KEY_Z, [&]() {
        // if back to menu, reset level and set menu state according to current level
        zoomOut = !zoomOut;
        if (zoomOut) {
            CamDist = currentLevel.maxCamDist;
        } else {
            CamDist = currentLevel.defaultCamDist;
        }
    });

    const std::unordered_map<int, HudSize> keyToSize = {
        {GLFW_KEY_4, H1920},
        {GLFW_KEY_5, H1600},
        {GLFW_KEY_6, H1120},
    };

    // logic for all menu keys is the same:
    // check if game state has changed and if so, update the level
    for (const auto &[key, sizeEnum]: keyToSize) {
        handleKey(key, [&]() {
            if (currentGameState != MenuL1 && currentGameState != MenuL2 && currentGameState != MenuL3) return;

            auto [width, height] = HudSizeMap.at(sizeEnum);

            // Resize the window
            // Get primary monitor work area (excluding taskbar/dock)
            GLFWmonitor *primary = glfwGetPrimaryMonitor();

            int monitorX, monitorY, monitorWidth, monitorHeight;
            glfwGetMonitorWorkarea(primary, &monitorX, &monitorY, &monitorWidth, &monitorHeight);

            if (monitorWidth < width || monitorHeight < height) return;

            currentHudSize = sizeEnum;
            windowWidth = width;
            windowHeight = height;
            shouldResizeWindow = true;
        });
    }

    // moves the view
    float deltaT = GameLogic();

    // defines the global parameters for the uniform
    glm::mat3 R = glm::mat3(
        glm::rotate(glm::mat4(1.0f), glm::radians(-30.0f), glm::vec3(0,1,0)) * // yaw (left)
        glm::rotate(glm::mat4(1.0f), glm::radians( -65.0f), glm::vec3(1,0,0))   // elevation up
    );
    glm::vec3 lightDir = glm::normalize(R * glm::vec3(0,0,1)); // TO sun
    // shader: L = normalize(-gubo.lightDir);

    GlobalUniformBufferObject gubo{};

    gubo.lightDir = lightDir;
    gubo.lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    gubo.eyePos = cameraPos;

    int instanceId;
    UniformBufferObject ubos{};

    // phong shader
    for (instanceId = 0; instanceId < SC.TI[0].InstanceCount; instanceId++) {
        ubos.mMat = SC.TI[0].I[instanceId].Wm;
        ubos.mvpMat = ViewPrj * ubos.mMat;
        ubos.nMat = glm::inverse(glm::transpose(glm::mat3(ubos.mMat)));


        SC.TI[0].I[instanceId].DS[0][0]->map(currentImage, &gubo, 0); // Set 0
        SC.TI[0].I[instanceId].DS[0][1]->map(currentImage, &ubos, 0); // Set 1
    }

    // phong shader
    for (instanceId = 0; instanceId < SC.TI[2].InstanceCount; instanceId++) {
        ubos.mMat = SC.TI[2].I[instanceId].Wm;
        ubos.mvpMat = ViewPrj * ubos.mMat;
        ubos.nMat = glm::inverse(glm::transpose(glm::mat3(ubos.mMat)));


        SC.TI[2].I[instanceId].DS[0][0]->map(currentImage, &gubo, 0); // Set 0
        SC.TI[2].I[instanceId].DS[0][1]->map(currentImage, &ubos, 0); // Set 1
    }

    // skybox
    SkyBoxUniformBufferObject sbubo{};
    sbubo.mvpMat = ViewPrj * glm::translate(glm::mat4(1), cameraPos) * glm::scale(glm::mat4(1), glm::vec3(100.0f));
    SC.TI[1].I[0].DS[0][0]->map(currentImage, &sbubo, 0);

    // overlay
    OverlayUniformBuffer oubo{};
    oubo.hudSizeVariant = static_cast<int>(currentHudSize);
    oubo.hudVariant = static_cast<int>(currentGameState);
    oubo.visible = true;
    DSOverlay.map(currentImage, &oubo, 0);

    // updates the FPS
    static float elapsedT = 0.0f;
    static int countedFrames = 0;

    countedFrames++;
    elapsedT += deltaT;
    if (elapsedT > 0.1f) {
        float Fps = (float) countedFrames / elapsedT;

        float oldTime = currentLevel.lowestTime;
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2);
        oss << "X: " << Pos.x << " Y: " << Pos.y << " Z: " << Pos.z << " Yaw: " << glm::degrees(Yaw) << " FPS: " << Fps
                << "\n";

        if (oldTime < 0.0f) {
            oss << "Highscore Timer: " << gameTimer.getTime() << "/NaN";
        } else {
            oss << "Highscore Timer: " << gameTimer.getTime() << "/" << oldTime;
        }

        txt.print(1.0f, 1.0f, oss.str(), 1, "CO", false, false, true, TAL_RIGHT, TRH_RIGHT, TRV_BOTTOM);

        elapsedT = 0.0f;
        countedFrames = 0;
    }

    txt.updateCommandBuffer();
}

float DrivingFrenzy::GameLogic() {
    Level &cl = currentLevel;

    // camera FOVy, near plane and far plane
    constexpr float FOVy = glm::radians(90.0f);
    constexpr float NEAR_PLANE = 0.1f;
    constexpr float FAR_PLANE = 500.f;

    // rotation and move speed of camrea
    constexpr float ROT_SPEED = glm::radians(120.0f);

    // constants for smooth acceleration
    constexpr float LAMBDA_VEL = 10.0f;
    constexpr float DAMPED_VEL_EPSILON = 0.0001f;

    // cam rotation boundaries
    constexpr float MAX_CAM_PITCH = M_PI / 3.0f;
    // TODO
    constexpr float MIN_CAM_PITCH = glm::radians(0.0f);

    // smooth camera factor
    constexpr float SMOOTH_CAM_LAMBDA = 10.0f;

    // cam reset
    constexpr float CAM_RESET_AFTER_SECONDS = 2.5f;
    constexpr float CAM_RESET_TARGET_YAW = M_PI;
    constexpr float CAM_RESET_LAMBDA = 1.5f;

    // static variables used to calculate view and world matrices
    // and other aspects of the movement and camera
    static float idleTime = 0.0f;

    static float CamPitch = glm::radians(40.0f);
    static float CamYaw = M_PI;
    // static float CamDist = 0.0f;
    static float CamRoll = 0.0f;

    static float SteeringAng = 0.0f;
    static float dampedVel = 0.0f;
    static float wheelRoll = 0.0f;

    // reset camera and car
    if (!cl.initialized) {
        idleTime = 0.0f;

        CamPitch = cl.startPitch;
        CamYaw = M_PI;
        CamDist = cl.defaultCamDist;
        CamRoll = 0.0f;

        SteeringAng = 0.0f;
        dampedVel = 0.0f;
        wheelRoll = 0.0f;

        cl.initialized = true;
    }

    // integration with the timers and the controllers
    float deltaT, deltaTclamp;
    auto m = glm::vec3(0.0f), r = glm::vec3(0.0f);
    bool fire = false;
    getSixAxis(deltaT, m, r, fire);

    deltaTclamp = std::clamp(deltaT, 0.0001f, 0.0166f);

    float MOVE_SPEED = fire ? cl.moveSpeedBoost : cl.moveSpeed;

    if (currentGameState != Playing) {
        m = glm::vec3(0.0f);
        dampedVel = 0.f;
    }

    // check if rotational input has occurred and update idle time for cam reset
    if (r.x != 0.0f || r.y != 0.0f || r.z != 0.0f) {
        idleTime = 0.0f;
    } else {
        idleTime += deltaTclamp;
    }
    // reset cam if idleTime surpasses threshold
    if (idleTime > CAM_RESET_AFTER_SECONDS) {
        float diff = fmod(CamYaw - CAM_RESET_TARGET_YAW + 3 * M_PI, 2 * M_PI) - M_PI;
        CamYaw -= diff * (1 - exp(-CAM_RESET_LAMBDA * deltaTclamp));
    }

    // calculate the steering angle based on the sterring speed, frame time and boundry values
    SteeringAng += -m.x * cl.steeringSpeed * deltaTclamp;
    SteeringAng = std::clamp(SteeringAng, -cl.maxSteeringAng, cl.maxSteeringAng);


    glm::vec3 CamPos = Pos;
    // static since it must persist and should only be initialized on the first call
    static glm::vec3 dampedCamPos = CamPos;


    float targetVel = MOVE_SPEED * m.z;

    // exponential smoothing of velocity
    float k = std::exp(-LAMBDA_VEL * deltaTclamp);
    dampedVel = targetVel * (1.0f - k) + dampedVel * k;

    // distance to move in the current frame
    float s = dampedVel * deltaTclamp;

    // threshold for moving
    if (std::abs(s) > DAMPED_VEL_EPSILON) {
        const float EPS_STEER = glm::radians(0.5f);
        const float MAX_DYAW = glm::radians(10.0f);

        // threshold for steering
        if (std::abs(SteeringAng) < EPS_STEER) {
            Pos.x -= std::sin(Yaw) * s;
            Pos.z -= std::cos(Yaw) * s;
        } else {
            float r = cl.wheelBase / std::tan(SteeringAng);
            r = std::clamp(r, -1000.0f, 1000.0f);

            float dYaw = std::clamp(s / r, -MAX_DYAW, MAX_DYAW);

            // calcaulte circle center
            float cx = Pos.x + r * std::cos(Yaw);
            float cz = Pos.z - r * std::sin(Yaw);
            //rotate aroun the circle
            Yaw -= dYaw;
            Pos.x = cx - r * std::cos(Yaw);
            Pos.z = cz + r * std::sin(Yaw);
        }

        // auto adjust steering to center when no steering input
        if (m.x == 0) {
            if (SteeringAng > cl.steeringSpeed * deltaTclamp) {
                SteeringAng -= cl.steeringSpeed * deltaTclamp;
            } else if (SteeringAng < -cl.steeringSpeed * deltaTclamp) {
                SteeringAng += cl.steeringSpeed * deltaTclamp;
            } else {
                SteeringAng = 0.0f;
            }
        }
    }

    // wheel radius in meters
    constexpr float wheelRadius = 0.5;

    // rotation change = arc length / radius
    wheelRoll -= s / wheelRadius;
    // wrap into [0, 2π]
    wheelRoll = std::fmod(wheelRoll, 2.0f * M_PI);
    if (wheelRoll < 0.0f)
        wheelRoll += 2.0f * M_PI;

    // Rotate camera around vehicle
    CamYaw += ROT_SPEED * deltaTclamp * r.y;
    CamPitch -= ROT_SPEED * deltaTclamp * r.x;
    CamRoll -= ROT_SPEED * deltaTclamp * r.z;
    CamDist -= MOVE_SPEED * deltaTclamp * m.y;

    // Clamp yaw (rotation around vehicle)
    // CamYaw = (CamYaw < 0.0f ? 0.0f : (CamYaw > 2*M_PI ? 2*M_PI : CamYaw));

    Yaw = std::fmod(Yaw, 2.0f * M_PI);
    if (Yaw < 0.0f)
        Yaw += 2.0f * M_PI;

    // infitie yaw rotation with modulo
    CamYaw = fmod(CamYaw, 2.0f * M_PI);
    if (CamYaw < 0.0f) CamYaw += 2.0f * M_PI;

    // limit pitch angle
    CamPitch = (CamPitch < MIN_CAM_PITCH ? MIN_CAM_PITCH : (CamPitch > MAX_CAM_PITCH) ? MAX_CAM_PITCH : CamPitch);

    // limit cam roll (not necessary since roll is not used
    CamRoll = (CamRoll < -M_PI ? -M_PI : (CamRoll > M_PI ? M_PI : CamRoll));

    // limit cam distance
    CamDist = std::clamp(CamDist, cl.minCamDist, cl.maxCamDist);
    // CamDist = (CamDist < MIN_CAM_DIST ? MIN_CAM_DIST : (CamDist > MAX_CAM_DIST ? MAX_CAM_DIST : CamDist));

    // calculate the target of the camera, set to be above the cars position
    glm::vec3 CamTarget = Pos + glm::vec3(glm::rotate(glm::mat4(1), Yaw, glm::vec3(0, 1, 0)) *
                                          glm::vec4(cl.camTargetDelta, 1));
    // calculate the actual positions of the camera based on the different angles and cam distance
    CamPos = CamTarget + glm::vec3(
                 glm::rotate(glm::mat4(1), Yaw + CamYaw, glm::vec3(0, 1, 0)) * glm::rotate(
                     glm::mat4(1), -CamPitch, glm::vec3(1, 0, 0)) *
                 glm::vec4(0, 0, CamDist, 1));


    // // dampens the camera movement
    dampedCamPos = CamPos * (1 - exp(-SMOOTH_CAM_LAMBDA * deltaTclamp)) +
                   dampedCamPos * exp(-SMOOTH_CAM_LAMBDA * deltaTclamp);
    // dampedCamPos = CamPos;
    cameraPos = dampedCamPos;
    ViewPrj = MakeViewProjectionLookAt(cameraPos, CamTarget, glm::vec3(0, 1, 0), CamRoll, FOVy, Ar, NEAR_PLANE,
                                       FAR_PLANE);

    // calculate world matrix for car
    World = MakeWorld(Pos, Yaw, Pitch, 0);
    glm::vec3 temp = World[3];
    temp[1] += 100;
    SC.I[SC.InstanceIds["axis"]]->Wm[3] = glm::vec4(temp, 1.f);
    // update the world matrix of all car parts (body + wheels)

    for (const auto &name: cl.carBodyNames) {
        int id = SC.InstanceIds[name];
        // adjust Wm by initial car location and transforms
        SC.I[id]->Wm = World * carParts[name].initial_Wm;
    }

    // wheel rotation based on velocity
    glm::mat4 wheelRotation = glm::rotate(glm::mat4(1.0f), wheelRoll, glm::vec3(1, 0, 0));

    for (const auto &name: cl.carWheelNames) {
        int id = SC.InstanceIds[name];

        // front wheels adjust yaw based on steering angle
        float extraYaw = std::find(cl.carFrontWheelNames.begin(), cl.carFrontWheelNames.end(), name) != cl.
                         carFrontWheelNames.end()
                             ? SteeringAng * 40.0f
                             : 0.0f;
        glm::mat4 steeringRotation = glm::rotate(glm::mat4(1.0f), glm::radians(extraYaw), glm::vec3(0, 1, 0));

        glm::mat4 wheelTransform = carParts[name].initial_Wm * steeringRotation * wheelRotation;
        SC.I[id]->Wm = World * wheelTransform;
    }

    // update bounding box of car and checkpoint
    const int id = SC.InstanceIds[cl.mainCarPartName];

    boundingBoxes[cl.mainCarPartName].updateFromWorldMatrix(SC.I[id]->Wm);
    const BoundingBox &carBB = boundingBoxes[cl.mainCarPartName];

    for (auto &[name, boundingBox]: boundingBoxes) {
        if (name == cl.mainCarPartName) continue;
        if (boundingBox.intersects(carBB)) {
            // if checkpoint is reached change pos to next checkpoint
            if (name == cl.checkpointName || name == cl.finalCheckpointName) {
                currentCheckpointIdx++;
                setCheckpointPos(currentCheckpointIdx);
                continue;
            }
            // std::cout << name << std::endl;
            if (collisionActive) {
                currentGameState = GameOver;
                gameTimer.pause();
            }
        }
    }

    // animate checkpoints
    float floatSpeed = 2.0f;
    float baseAmplitude = 0.3f;
    float rotateSpeed = glm::radians(30.0f);

    float time = glfwGetTime();

    for (auto &name: std::vector{cl.checkpointName, cl.finalCheckpointName}) {
        const int id = SC.InstanceIds[name];
        const auto &bbox = boundingBoxes.at(name);

        // get height of model and center for uniform animation and later moving object to origin
        float modelHeight = 2.0f * bbox.halfExtents.y;
        glm::vec3 center = bbox.center;

        float scaledAmplitude = baseAmplitude / modelHeight;

        // sinusoidal up and down movement
        float offsetY = sin(time * floatSpeed) * scaledAmplitude;
        float rotation = time * rotateSpeed;

        // move checkpoint to origin, apply transform and then move back
        glm::mat4 anim = glm::translate(glm::mat4(1.0f), center);
        anim = glm::rotate(anim, rotation, glm::vec3(0, 1, 0));
        anim = glm::translate(anim, glm::vec3(0, offsetY, 0));
        anim = glm::translate(anim, -center);

        if (name == cl.checkpointName) {
            SC.I[id]->Wm = anim * baseTransformCheckpoint;
        } else {
            SC.I[id]->Wm = anim * baseTransformFinalCheckpoint;
        }
    }

    if (cl.levelId != Showcase) return deltaT;

    static float spin = 0;

    spin += glm::radians(deltaTclamp * 45.f);

    for (const auto& pedestal : cl.pedestals) {
        auto pivot = glm::vec3(pedestal.initial_Wm[3]);

        glm::mat4 Pivot = MakeWorld(pivot, spin, 0.0f, 0.0f) * glm::translate(glm::mat4(1.0f), -pivot);

        int id = SC.InstanceIds[pedestal.name];
        SC.I[id]->Wm = Pivot * pedestal.initial_Wm;

        for (const auto& car_part : pedestal.car_body_parts) {
            id = SC.InstanceIds[car_part.name];
            SC.I[id]->Wm = Pivot * car_part.initial_Wm;
        }

        for (const auto& wheel : pedestal.car_wheels) {
            id = SC.InstanceIds[wheel.name];
            SC.I[id]->Wm = Pivot * wheel.initial_Wm ;
        }
    }

    return deltaT;
}
