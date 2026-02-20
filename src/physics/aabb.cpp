#include "aabb.hpp"

AABB *get_aabb(Mesh *mesh) {
  glm::vec3 max = glm::vec3(-INFINITY, -INFINITY, -INFINITY);
  glm::vec3 min = glm::vec3(INFINITY, INFINITY, INFINITY);
  float *maxp = glm::value_ptr(max);
  float *minp = glm::value_ptr(min);
  AABB *aabb = (AABB*)malloc(sizeof(AABB));
  for (auto &vert : mesh->verts) {
    float *cur_vert = glm::value_ptr(vert);
    for (int i = 0; i < 3; i++) {
      if (cur_vert[i] > maxp[i]) {
        maxp[i] = cur_vert[i];
      } else if (cur_vert[i] < minp[i]) {
        minp[i] = cur_vert[i];
      }
    }
  }
  aabb->max = max;
  aabb->min = min;
  return aabb;
}

Mesh *get_aabb_mesh(AABB *aabb) {
  Mesh *cube = get_cube();
  for (auto &vert : cube->verts) {
    float *cur_vert = glm::value_ptr(vert);
    for (int i = 0; i < 3; i++) {
      if (cur_vert[i] > 0.0) {
        cur_vert[i] = aabb->max[i];
      } else if (cur_vert[i] < 0.0) {
        cur_vert[i] = aabb->min[i];
      }
    }
  }
  return cube;
}

AABB *clone(AABB *a) {
  AABB *aabb = (AABB*)calloc(1, sizeof(AABB));
  memcpy(aabb, a, sizeof(AABB));
  return aabb;
}

AABB *sum_aabb(AABB *a, AABB *b) {
  AABB *aabb = (AABB*)malloc(sizeof(AABB));
  aabb->max = glm::vec3(-INFINITY);
  aabb->min = glm::vec3(INFINITY);
  for (int x = 0; x < 4; x++) {
    float *cur_vert;
    switch (x) {
      case 0:
        cur_vert = glm::value_ptr(a->max);
        break;
      case 1:
        cur_vert = glm::value_ptr(a->min);
        break;
      case 2:
        cur_vert = glm::value_ptr(b->max);
        break;
      case 3:
        cur_vert = glm::value_ptr(b->min);
        break;
    }
    for (int i = 0; i < 3; i++) {
      if (cur_vert[i] > aabb->max[i]) {
        aabb->max[i] = cur_vert[i];
      } else if (cur_vert[i] < aabb->min[i]) {
        aabb->min[i] = cur_vert[i];
      }
    }
  }
  return aabb;
}

u32 intersect_ray_aabb(AABB *aabb, glm::vec3 origin, glm::vec3 direction, float *dist) {
  float t1 = (aabb->min.x - origin.x) / direction.x;
  float t2 = (aabb->max.x - origin.x) / direction.x;
  float t3 = (aabb->min.y - origin.y) / direction.y;
  float t4 = (aabb->max.y - origin.y) / direction.y;
  float t5 = (aabb->min.z - origin.z) / direction.z;
  float t6 = (aabb->max.z - origin.z) / direction.z;

  float tmin = fmax(fmax(fmin(t1, t2), fmin(t3, t4)), fmin(t5, t6));
  float tmax = fmin(fmin(fmax(t1, t2), fmax(t3, t4)), fmax(t5, t6));

  *dist = tmax;

  if (tmax < 0.0 || tmin > tmax) {
    return 0;
  }

  if (tmin < 0.0f) {
    return 1;
  }

  *dist = tmin;
  return 1;
}

glm::vec3 get_center(AABB *aabb) {
  glm::vec3 center = (aabb->min + aabb->max) / 2.0f;
  // printf("%s\n", to_string(aabb->min).c_str());
  // printf("%s\n", to_string(aabb->max).c_str());
  // printf("%s\n", to_string(center).c_str());
  return center;
}
