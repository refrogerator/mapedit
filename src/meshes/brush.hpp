#pragma once

#include <glm/glm.hpp>
#include "../meshes/editing.hpp"
#include "../physics/aabb.hpp"
#include <glad/glad.h>

struct Selection {
  u32 obj;
  std::vector<u32> faces;
  std::vector<u32> verts;
};

struct Brush {
  glm::vec3 origin;
  glm::vec3 rotation;
  glm::vec3 scale;
  Mesh *mesh;
  UploadedMesh *uploaded;
  AABB *aabb;
  UploadedMesh *aabb_debug;
  u32 *mat;
};

struct BrushList {
  u64 len;
  u64 max;
  Brush *data;
};

void update_mesh(Mesh *mesh, UploadedMesh *umesh);

UploadedMesh *upload_mesh(Mesh *mesh);

void render_mesh(UploadedMesh *mesh);

Brush *new_brush();

glm::mat4 get_brush_matrix(Brush *brush);

void render_brush(Brush *brush);

u32 select_brush_face(Brush *brush, glm::vec3 origin, glm::vec3 dir);

u32 intersect_brush(Brush *brush, glm::vec3 origin, glm::vec3 dir, float *dist);

BrushList new_brushlist();

BrushList remove_brush(BrushList brushes, u32 index);

void render_brushes_points(BrushList brushes, std::vector<Selection> selected);

void render_brushes(BrushList brushes, std::vector<Selection> selected);

Brush *get_brush(BrushList brushes, u32 index);

Brush *get_brush_ptr(BrushList brushes, u32 index);

void update_brush(Brush *brush);
