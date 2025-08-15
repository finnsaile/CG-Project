#include "modules/GhostReplay.hpp"

#include <fstream>
#include <iostream>

void GhostReplay::record(const glm::mat4& bodyWm, const glm::mat4 wheelsWm[4]) {
    frames.push_back({bodyWm, {wheelsWm[0], wheelsWm[1], wheelsWm[2], wheelsWm[3]}});
}

size_t GhostReplay::getFrameCount() const {
    return frames.size();
}

GhostFrame GhostReplay::getFrameAtTime(float time) const {
    size_t frameIndex = static_cast<size_t>(time * fps);
    if (frameIndex >= frames.size()) {
        return frames.back();
    }
    return frames[frameIndex];
}

void GhostReplay::clear() {
    frames.clear();
}

void GhostReplay::saveToFile(const std::string& filename) const {
    std::ofstream out(filename, std::ios::binary);

    if (!out) return;

    uint32_t frameCount = static_cast<uint32_t>(frames.size());
    out.write(reinterpret_cast<const char*>(&frameCount), sizeof(uint32_t));

    for (const auto& frame : frames) {
        out.write(reinterpret_cast<const char *>(&frame.bodyWm), sizeof(glm::mat4));
        for (const auto & i : frame.wheelsWm) {
            out.write(reinterpret_cast<const char *>(&i), sizeof(glm::mat4));
        }
    }

    out.close();
}


void GhostReplay::loadFromFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return;

    uint32_t frameCount;
    in.read(reinterpret_cast<char*>(&frameCount), sizeof(uint32_t));

    frames.resize(frameCount);
    for (auto& frame : frames) {
        in.read(reinterpret_cast<char*>(&frame.bodyWm), sizeof(glm::mat4));
        for (auto & i : frame.wheelsWm)
            in.read(reinterpret_cast<char*>(&i), sizeof(glm::mat4));
    }

    in.close();
}