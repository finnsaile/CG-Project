#ifndef BOUNDINGBOX_HPP
#define BOUNDINGBOX_HPP

#include <iostream>
#include <glm/glm.hpp>


class BoundingBox {
public:
    std::string name;
    glm::vec3 localCenter;
    glm::vec3 localHalfExtents;
    glm::vec3 center;
    glm::vec3 axes[3];
    glm::vec3 halfExtents;

    BoundingBox() = default;

    BoundingBox(std::string name, const glm::mat4 &Wm, const std::vector<unsigned char> &vertices);

    void updateFromWorldMatrix(const glm::mat4 &Wm);

    bool intersects(const BoundingBox &other) const;

private:
    std::vector<glm::vec3> extractPositionsFromBuffer(
        const std::vector<unsigned char> &vertices,
        size_t stride,
        size_t positionOffset);

    void computeLocalHalfExtents(const std::vector<glm::vec3> &positions);
};


#endif
