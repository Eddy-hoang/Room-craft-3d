#include "roomcraft/scene/Transform.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <cmath>

namespace roomcraft::scene {

Transform::Transform(const glm::vec3& pos,
                     const glm::vec3& rotDeg,
                     const glm::vec3& scl)
    : position(pos)
    , rotationDegrees(rotDeg)
    , scale(scl)
{
}

glm::mat4 Transform::getModelMatrix() const
{
    // Thứ tự nhân ma trận cột trong GLM (từ phải sang trái khi nhân với vector cột p_local):
    //   p_world = T * R_y * R_x * R_z * S * p_local
    // Tương đương: Scale trước -> xoay quanh Z -> xoay quanh X -> xoay quanh Y -> Tịnh tiến.
    glm::mat4 model(1.0f);
    model = glm::translate(model, position);
    model = glm::rotate(model, glm::radians(rotationDegrees.y), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(rotationDegrees.x), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(rotationDegrees.z), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, scale);
    return model;
}

void Transform::reset()
{
    position = glm::vec3(0.0f, 0.0f, 0.0f);
    rotationDegrees = glm::vec3(0.0f, 0.0f, 0.0f);
    scale = glm::vec3(1.0f, 1.0f, 1.0f);
}

bool Transform::isValidScale(const glm::vec3& candidateScale, float minScale)
{
    return std::isfinite(candidateScale.x) &&
           std::isfinite(candidateScale.y) &&
           std::isfinite(candidateScale.z) &&
           candidateScale.x >= minScale &&
           candidateScale.y >= minScale &&
           candidateScale.z >= minScale;
}

bool Transform::setScaleSafe(const glm::vec3& candidateScale, float minScale)
{
    if (!isValidScale(candidateScale, minScale)) {
        return false;
    }
    scale = candidateScale;
    return true;
}

} // namespace roomcraft::scene
