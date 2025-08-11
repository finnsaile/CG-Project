#include "modules/BoundingBox.hpp"

BoundingBox::BoundingBox(std::string name, const glm::mat4 &Wm,
                         const std::vector<unsigned char> &vertices) : name(name) {
    std::vector<glm::vec3> positions = extractPositionsFromBuffer(vertices, 32, 0);
    // compute the LOCAL half extents and centers needed for the bounding boxes
    computeLocalHalfExtents(positions);

    // forgiving bounding boxes
    localHalfExtents.x *= 0.95f;

    updateFromWorldMatrix(Wm);
}

void BoundingBox::updateFromWorldMatrix(const glm::mat4 &Wm) {
    center = glm::vec3(Wm * glm::vec4(localCenter, 1.0f));

    // extract axes columns
    const auto col0 = glm::vec3(Wm[0]);
    const auto col1 = glm::vec3(Wm[1]);
    const auto col2 = glm::vec3(Wm[2]);

    // and their scaling factors
    const glm::vec3 scale = {
        glm::length(col0),
        glm::length(col1),
        glm::length(col2)
    };

    axes[0] = glm::normalize(col0);
    axes[1] = glm::normalize(col1);
    axes[2] = glm::normalize(col2);

    // apply scaling to local half extends
    halfExtents = localHalfExtents * scale;
}

bool BoundingBox::intersects(const BoundingBox &other) const {
    const float EPSILON = 1e-6f;

    glm::mat3 R;
    glm::mat3 AbsR;

    // Compute rotation matrix from B to A
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++) {
            R[i][j] = glm::dot(this->axes[i], other.axes[j]);
            AbsR[i][j] = std::abs(R[i][j]) + EPSILON;
        }

    // Vector between centers
    glm::vec3 t = other.center - this->center;
    // Express t in A's local space
    t = glm::vec3(glm::dot(t, this->axes[0]),
                  glm::dot(t, this->axes[1]),
                  glm::dot(t, this->axes[2]));

    // Test axes A0, A1, A2
    for (int i = 0; i < 3; i++) {
        float ra = this->halfExtents[i];
        float rb = other.halfExtents[0] * AbsR[i][0] + other.halfExtents[1] * AbsR[i][1] + other.halfExtents[2] * AbsR[
                       i][2];
        if (std::abs(t[i]) > ra + rb) return false;
    }

    // Test axes B0, B1, B2
    for (int i = 0; i < 3; i++) {
        float ra = this->halfExtents[0] * AbsR[0][i] + this->halfExtents[1] * AbsR[1][i] + this->halfExtents[2] * AbsR[
                       2][i];
        float rb = other.halfExtents[i];
        if (std::abs(t[0] * R[0][i] + t[1] * R[1][i] + t[2] * R[2][i]) > ra + rb) return false;
    }

    // Test cross products
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            float ra = this->halfExtents[(i + 1) % 3] * AbsR[(i + 2) % 3][j] + this->halfExtents[(i + 2) % 3] * AbsR[
                           (i + 1) % 3][j];
            float rb = other.halfExtents[(j + 1) % 3] * AbsR[i][(j + 2) % 3] + other.halfExtents[(j + 2) % 3] * AbsR[i][
                           (j + 1) % 3];
            float val = std::abs(t[(i + 2) % 3] * R[(i + 1) % 3][j] - t[(i + 1) % 3] * R[(i + 2) % 3][j]);
            if (val > ra + rb) return false;
        }
    }

    return true;
}

std::vector<glm::vec3> BoundingBox::extractPositionsFromBuffer(
    const std::vector<unsigned char> &vertices,
    size_t stride = 32,
    size_t positionOffset = 0) {
    std::vector<glm::vec3> positions;
    const size_t vertexCount = vertices.size() / stride;
    positions.reserve(vertexCount);

    for (size_t i = 0; i < vertexCount; ++i) {
        const unsigned char *base = &vertices[i * stride + positionOffset];
        float x = *reinterpret_cast<const float *>(base);
        float y = *reinterpret_cast<const float *>(base + 4);
        float z = *reinterpret_cast<const float *>(base + 8);
        positions.emplace_back(x, y, z);
    }

    return positions;
}

void BoundingBox::computeLocalHalfExtents(const std::vector<glm::vec3> &positions) {
    glm::vec3 minCorner(FLT_MAX);
    glm::vec3 maxCorner(-FLT_MAX);

    for (const auto &p: positions) {
        minCorner = glm::min(minCorner, p);
        maxCorner = glm::max(maxCorner, p);
    }

    localHalfExtents = (maxCorner - minCorner) * 0.5f;
    localCenter = (maxCorner + minCorner) * 0.5f;
}
