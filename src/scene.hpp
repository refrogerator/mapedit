#pragma once

#include "common.h"
#include <glm/glm.hpp>
#include <vector>
#include "meshes/brush.hpp"
#include "renderer/gltf.hpp"

struct Object {
  glm::vec3 pos;
  glm::vec3 rotation;
  glm::vec3 scale;
  char *classname;
  Model *model;
};

enum NodeType {
  node_type_object,
  node_type_brush,
};

struct SceneNode {
  enum NodeType type;
  union {
    Object object;
    Brush brush;
  };
};

u32 select_node(std::vector<SceneNode> scene, glm::vec3 origin, glm::vec3 dir, float *_dist);

void remove_node(std::vector<SceneNode> *scene, int idx);
