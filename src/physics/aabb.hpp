#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include "../common.h"
#include "../meshes/editing.hpp"

struct AABB {
  glm::vec3 max;
  glm::vec3 min;
};

AABB *get_aabb(Mesh *mesh);

Mesh *get_aabb_mesh(AABB *aabb);

AABB *clone(AABB *a);

AABB *sum_aabb(AABB *a, AABB *b);

u32 intersect_ray_aabb(AABB *aabb, glm::vec3 origin, glm::vec3 direction, float *dist);

glm::vec3 get_center(AABB *aabb);
