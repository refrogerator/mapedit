struct Camera {
  glm::vec3 origin;
  glm::quat rotation;
  float pitch;
  float yaw;
  float fov;
};

namespace cam {
  void translate_x(struct Camera *cam, float amt) {
    // glm::mat4 viewc = glm::toMat4(cam->rotation);
    glm::vec3 temp = glm::vec3(glm::vec4(-1.0f, 0.0f, 0.0f, 1.0f) * cam->rotation );
    temp = glm::normalize(temp);
    temp *= amt;
    cam->origin += temp;
  }

  void translate_z(struct Camera *cam, float amt) {
    glm::vec3 temp = glm::vec3(glm::vec4(0.0f, 0.0f, -1.0f, 1.0f) * cam->rotation );
    temp = glm::normalize(temp);
    temp *= amt;
    cam->origin += temp;
  }

  void translate_y(struct Camera *cam, float amt) {
    cam->origin.y += amt;
  }

  glm::quat rotation(struct Camera *cam) {
    glm::quat pitch = glm::quat(glm::vec3(cam->pitch, 0.0f, 0.0f));
    glm::quat yaw = glm::quat(glm::vec3(0.0f, cam->yaw, 0.0f));
    return pitch * yaw;
  }
}

// glm::mat4 get_view(struct Camera *cam) {
//   glm::mat4 view = glm::toMat4(cam->rotation);
//   view = glm::cam_translate(view, cam->origin);
//   return view;
// }
