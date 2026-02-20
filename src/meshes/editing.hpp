#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/intersect.hpp>
#include <glm/gtx/string_cast.hpp>
#include <vector>
#include "../common.h"

struct Face {
  std::vector<u32> indices;
  std::vector<glm::vec2> uvs;
};

struct Mesh {
  std::vector<Face> faces;
  std::vector<glm::vec3> verts;
};

struct UploadedMesh {
  u32 vao;
  u32 vbo;
  u32 visible;
  u32 num_verts;
  u32 num_inds;
};

glm::vec2 *get_default_uvs();

Mesh *get_cube();

/*u32 *get_lines(Mesh *mesh, u32 *num) {
  u32 fort = 0;
  for (int i = 0; i < mesh->facec; i++) {
    fort += mesh->facev[i].vertc;
  }
  *num = fort;
  u32 *output = (u32*)malloc(sizeof(u32) * fort * 2);
  u32 nite = 0;
  for (int i = 0; i < mesh->facec; i++) {
    Face *face = &mesh->facev[i];
    for (int y = 0; y < face->vertc - 1; y++) {
      output[nite] = face->vertv[y];
      nite += 1;
      output[nite] = face->vertv[y + 1];
      nite += 1;
    }
    output[nite] = face->vertv[face->vertc - 1];
    nite += 1;
    output[nite] = face->vertv[0];
    nite += 1;
  }
  return output;
}
*/

std::vector<u32> get_tris_face(Face *face);

u32 emit_vert(float *out, u32 index_, glm::vec3 pos, glm::vec3 normal, glm::vec2 uv, u32 facei);

float *get_tris(Mesh *mesh, u32 *num);

glm::vec3 get_normal_face(Mesh *mesh, Face *face);

void move_face_along_normal(Mesh *mesh, u32 facei, i32 dir, i32 grid);

void move_faces(Mesh *mesh, std::vector<u32> faceis, glm::vec3 offs);

u32 extrude_face(Mesh *mesh, u32 facei);

u32 intersect_faces(Mesh *mesh, glm::mat4 transform, glm::vec3 origin, glm::vec3 direction, float *dist);

void gen_uvs(Mesh *mesh);

glm::vec3 get_face_center(Mesh *mesh, u32 facei);

// AABB *get_face_aabb(Mesh *mesh, u32 facei) {
//   AABB *aabb = (AABB*)malloc(sizeof(AABB));
//   aabb->max = glm::vec3(-INFINITY);
//   aabb->min = glm::vec3(INFINITY);
//   if (facei >= mesh->faces.size()) {
//     return aabb;
//   }
//   for (int x = 0; x < mesh->faces[facei].indices.size(); x++) {
//     float *cur_vert = mesh->verts[mesh->faces[facei].indices[x]];
//     for (int i = 0; i < 3; i++) {
//       if (cur_vert[i] > aabb->max[i]) {
//         aabb->max[i] = cur_vert[i];
//       } else if (cur_vert[i] < aabb->min[i]) {
//         aabb->min[i] = cur_vert[i];
//       }
//     }
//   }
//   return aabb;
// }

void recenter_mesh(Mesh *mesh, glm::vec3 origin, glm::vec3 center);
