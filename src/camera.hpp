#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

struct Camera {
  glm::vec3 origin;
  glm::quat rotation;
  float pitch;
  float yaw;
  float fov;
};

namespace cam {
  void translate_x(struct Camera *cam, float amt);

  void translate_z(struct Camera *cam, float amt);

  void translate_y(struct Camera *cam, float amt);

  glm::quat rotation(struct Camera *cam);
}

