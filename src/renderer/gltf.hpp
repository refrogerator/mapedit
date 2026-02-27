#pragma once

#include <cgltf.h>
#include "../common.h"
#include "../physics/aabb.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/intersect.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glad/glad.h>
#include <stb_image.h>

int gltf_get_count(cgltf_type type);

int gltf_type_to_gl_type(cgltf_component_type type);

struct Triangle {
  glm::vec3 v0;
  glm::vec3 v1;
  glm::vec3 v2;
};

struct Physics {
  std::vector<Triangle> tris;
};

struct Material {
  u32 tex;
  glm::vec3 albedo;
};

struct GltfMesh {
  u32 elements;
  u32 vao;
  cgltf_mesh *mesh;
  Material material;
  glm::mat4 transform;
  char *name;
  AABB *aabb;
};

struct Model {
  cgltf_data *data;
  GltfMesh *meshes;
  glm::mat4 transform;
  u32 count;
  std::string name;
};

int gltf_intersect_mesh(GltfMesh *mesh, glm::mat4 transform, glm::vec3 origin, glm::vec3 direction, float *dist);

// int gltf_intersect_model(Model *model, glm::vec3 origin, glm::vec3 direction, float *dist);

GltfMesh gltf_upload_mesh(cgltf_node *node);

Model gltf_upload_model(cgltf_data *data);

void gltf_mesh_get_physics(GltfMesh *_mesh, Physics *chud);

Physics gltf_model_get_physics(Model *model);

// int gltf_intersect_mesh(GltfMesh *_mesh, glm::mat4 tr, glm::vec3 origin, glm::vec3 direction, float *_dist) { }

int gltf_intersect_model(Model *model, glm::vec3 origin, glm::vec3 direction, float *_dist);

void render_model(Model *model);

void render_model_jort(Model *model, int index);
