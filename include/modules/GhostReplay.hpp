#ifndef GHOSTREPLAY_HPP
#define GHOSTREPLAY_HPP

#include <vector>
#include <glm/glm.hpp>

struct GhostFrame {
    glm::mat4 bodyWm;
    glm::mat4 wheelsWm[4];
};

class GhostReplay {
    std::vector<GhostFrame> frames;

public:
    float fps = 144;

    GhostReplay() = default;

    void record(const glm::mat4 &bodyWm, const glm::mat4 wheelsWm[4]);

    size_t getFrameCount() const;

    GhostFrame getFrameAtTime(float time) const;

    void clear();

    void saveToFile(const std::string &filename) const;

    void loadFromFile(const std::string &filename);
};

#endif
