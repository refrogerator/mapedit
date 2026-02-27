#include "editing.hpp"
#include <bit>
#include <ranges>

glm::vec2 *get_default_uvs() {
  glm::vec2 uv0 = glm::vec2(0.0, 0.0);
  glm::vec2 uv1 = glm::vec2(0.0, 1.0);
  glm::vec2 uv2 = glm::vec2(1.0, 1.0);
  glm::vec2 uv3 = glm::vec2(1.0, 0.0);

  glm::vec2 *uvs = (glm::vec2*)malloc(sizeof(glm::vec2) * 4);
  uvs[0] = uv0;
  uvs[1] = uv1;
  uvs[2] = uv2;
  uvs[3] = uv3;
  return uvs;
}

std::vector<Edge> get_edges(std::vector<Face> &faces) {
    std::vector<Edge> edges;

    for (Face &face : faces) {
        for (u32 i = 0; i < face.indices.size(); i++) {
            u32 a = face.indices[i];
            u32 b = face.indices[(i + 1) % face.indices.size()];
            bool found = false;
            for (Edge &edge : edges) {
                if ((edge.a == a && edge.b == b) ||
                    (edge.a == b && edge.b == a)) {
                    found = true;
                }
            }
            if (!found) {
                 edges.push_back(Edge{a, b});
            }
        }
    }

    return edges;
}

Mesh *get_cube() {
  Mesh *mesh = new Mesh;
  mesh->verts = {
    glm::vec3(-0.5, -0.5, -0.5), // fbl 0
    glm::vec3(-0.5,  0.5, -0.5), // ftl 1
    glm::vec3( 0.5,  0.5, -0.5), // ftr 2
    glm::vec3( 0.5, -0.5, -0.5), // fbr 3
    glm::vec3(-0.5, -0.5,  0.5), // bbl 4
    glm::vec3(-0.5,  0.5,  0.5), // btl 5
    glm::vec3( 0.5,  0.5,  0.5), // btr 6
    glm::vec3( 0.5, -0.5,  0.5), // bbr 7
  };

  std::vector<glm::vec2> uvs = {
    glm::vec2(0.0, 0.0),
    glm::vec2(0.0, 1.0),
    glm::vec2(1.0, 1.0),
    glm::vec2(1.0, 0.0),
  };

  mesh->faces = {
    Face {{0, 1, 2, 3}, uvs},
    Face {{7, 6, 5, 4}, uvs},
    Face {{4, 5, 1, 0}, uvs},
    Face {{3, 2, 6, 7}, uvs},
    Face {{1, 5, 6, 2}, uvs},
    Face {{3, 7, 4, 0}, uvs},
  };

  mesh->edges = get_edges(mesh->faces);

  return mesh;
}

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

std::vector<u32> get_tris_face(Face *face) {
  std::vector<u32> output;

  if (face->indices.size() == 4) {
    output.push_back(face->indices[0]);
    output.push_back(face->indices[1]);
    output.push_back(face->indices[2]);
    output.push_back(face->indices[2]);
    output.push_back(face->indices[3]);
    output.push_back(face->indices[0]);
  }

  return output;
}

void emit_vert(std::vector<float> *out, glm::vec3 pos, glm::vec3 normal, glm::vec2 uv, u32 facei) {
  out->push_back(pos.x);
  out->push_back(pos.y);
  out->push_back(pos.z);
  out->push_back(normal.x);
  out->push_back(normal.y);
  out->push_back(normal.z);
  out->push_back(uv.x);
  out->push_back(uv.y);
  out->push_back(std::bit_cast<float>(facei));
}

std::vector<float> get_tris(Mesh *mesh) {
  // u32 fort = 0;
  // for (Face &face : mesh->faces) {
  //   fort += face.indices.size() - 2;
  // }
  // *num = fort;

  u32 vert_size = 3 * sizeof(float) + 3 * sizeof(float) + 2 * sizeof(float) + 1 * sizeof(u32);
  // std::vector<float> out = (float*)malloc(vert_size * 3 * fort);
  std::vector<float> out;

  for (auto [i, face] : std::views::enumerate(mesh->faces)) {
    // u32 *tris =  get_tris_face(face);
    // for (int i = 0; i < 6; i++) {
    //   out[nite] = tris[i];
    //   nite++;
    // }
    glm::vec3 v1 = mesh->verts[face.indices[2]] - mesh->verts[face.indices[1]];
    glm::vec3 v2 = mesh->verts[face.indices[0]] - mesh->verts[face.indices[1]];
    glm::vec3 normal = glm::normalize(glm::cross(v1, v2));

    if (face.indices.size() == 4) {
      emit_vert(&out, mesh->verts[face.indices[0]], normal, face.uvs[0], i);
      emit_vert(&out, mesh->verts[face.indices[1]], normal, face.uvs[1], i);
      emit_vert(&out, mesh->verts[face.indices[2]], normal, face.uvs[2], i);

      emit_vert(&out, mesh->verts[face.indices[2]], normal, face.uvs[2], i);
      emit_vert(&out, mesh->verts[face.indices[3]], normal, face.uvs[3], i);
      emit_vert(&out, mesh->verts[face.indices[0]], normal, face.uvs[0], i);
    }
  }

  return out;
}

std::vector<float> get_render_edges(Mesh *mesh) {
    std::vector<glm::vec3> temp_verts = mesh->verts;

    for (Face &face : mesh->faces) {
        glm::vec3 v1 = mesh->verts[face.indices[2]] - mesh->verts[face.indices[1]];
        glm::vec3 v2 = mesh->verts[face.indices[0]] - mesh->verts[face.indices[1]];
        glm::vec3 normal = glm::normalize(glm::cross(v1, v2));

        for (u32 i : face.indices) {
            temp_verts[i] += normal * 0.01f;
        }
    }

    std::vector<float> ret;

    for (Edge &edge : mesh->edges) {
        ret.push_back(temp_verts[edge.a].x);
        ret.push_back(temp_verts[edge.a].y);
        ret.push_back(temp_verts[edge.a].z);
        ret.push_back(std::bit_cast<float>(edge.a));
        ret.push_back(temp_verts[edge.b].x);
        ret.push_back(temp_verts[edge.b].y);
        ret.push_back(temp_verts[edge.b].z);
        ret.push_back(std::bit_cast<float>(edge.b));
    }

    return ret;
}

glm::vec3 get_normal_face(Mesh *mesh, Face *face) {
  glm::vec3 v1 = mesh->verts[face->indices[2]] - mesh->verts[face->indices[1]];
  glm::vec3 v2 = mesh->verts[face->indices[0]] - mesh->verts[face->indices[1]];
  glm::vec3 normal = glm::normalize(glm::cross(v1, v2));
  return normal;
}

void move_face_along_normal(Mesh *mesh, u32 facei, i32 dir, i32 grid) {
  if (facei >= mesh->faces.size()) {
    return;
  }
  Face *face = &mesh->faces[facei];
  glm::vec3 normal = get_normal_face(mesh, face);

  float amount = (float)dir;


  float balls = 256.0 / pow(2.0f, (float)grid);
  amount /= balls;

  glm::vec3 normal_scaled = normal * amount;

  for (u32 &index : face->indices) {
    glm::vec3 vert = mesh->verts[index];
    glm::vec3 moved = vert + normal_scaled;

    mesh->verts[index] = moved;
  }
}

void move_faces(Mesh *mesh, std::vector<u32> faceis, glm::vec3 offs) {
  std::vector<u32> unique_inds;
  for (u32 facei : faceis) {
    if (facei >= mesh->faces.size()) {
      return;
    }
    Face *face = &mesh->faces[facei];
    for (u32 index : face->indices) {
      int unique = 0;
      for (u32 unique_ind : unique_inds) {
        if (unique_ind == index) {
          unique = 1;
        }
      }
      if (!unique) {
        unique_inds.push_back(index);
      }
    }
  }

  for (u32 index : unique_inds) {
    glm::vec3 vert = mesh->verts[index];
    glm::vec3 moved = vert + offs;

    mesh->verts[index] = moved;
  }
}

u32 extrude_face(Mesh *mesh, u32 facei) {
  if (facei >= mesh->faces.size()) {
    return -1;
  }


  // mesh->facec += face->vertc;
  // mesh->facev = (Face*)realloc(mesh->facev, sizeof(Face) * mesh->facec);

  //face = &mesh->faces[facei];

  // mesh->vertc = mesh->vertc + face->vertc;
  // mesh->vertv = (float*)realloc(mesh->vertv, sizeof(float) * 3 * mesh->vertc);

  // memcpy(&mesh->vertv[3 * (lasti_old - 1)], &mesh->vertv[3 * face->vertv[0]], sizeof(float) * 3 * face->vertc);

  Face *face = &mesh->faces[facei];
  u32 lasti_old = mesh->verts.size();

  for (u32 index : face->indices) {
    glm::vec3 vert = mesh->verts[index];
    mesh->verts.push_back(vert);
  }


  std::vector<glm::vec2> uvs = {
    glm::vec2(0.0, 0.0),
    glm::vec2(0.0, 1.0),
    glm::vec2(1.0, 1.0),
    glm::vec2(1.0, 0.0),
  };

  int joe = face->indices.size();

  for (int i = 0; i < joe; i++) {
    face = &mesh->faces[facei];
    Face newface;
    newface.uvs = uvs;

    if (i != face->indices.size() - 1) {
      int jort = face->indices[i + 1];
      newface.indices.push_back(jort);
      newface.indices.push_back(lasti_old + i + 1);
    } else {
      int jort = face->indices[0];
      newface.indices.push_back(jort);
      newface.indices.push_back(lasti_old);
    }
    int jort = face->indices[i];
    newface.indices.push_back(lasti_old + i);
    newface.indices.push_back(jort);

    mesh->faces.push_back(newface);
  }

  for (int i = 0; i < face->indices.size(); i++) {
    mesh->faces[facei].indices[i] = lasti_old + i;
  }

  // TODO improve performance by just adding the new edges
  mesh->edges = get_edges(mesh->faces);

  return mesh->faces.size() - 1;
}

u32 intersect_faces(Mesh *mesh, glm::mat4 transform, glm::vec3 origin, glm::vec3 direction, float *dist) {
  u32 ret = -1;
  float best_dist = INFINITY;
  float _dist = INFINITY;
  for (size_t i = 0; Face &face : mesh->faces) {
    glm::vec3 normal = get_normal_face(mesh, &face);
    float dot = glm::dot(normal, direction);
    if (dot > 0.0) {
      i++;
      continue;
    }

    std::vector<u32> tris = get_tris_face(&face);
    u32 num_tris = face.indices.size() - 2;
    glm::vec2 test_result;
    for (int t = 0; t < num_tris; t++) {
      _dist = INFINITY;
      glm::vec3 v0 =  transform * glm::vec4(mesh->verts[tris[t * 3 + 0]], 1.0);
      glm::vec3 v1 =  transform * glm::vec4(mesh->verts[tris[t * 3 + 1]], 1.0);
      glm::vec3 v2 =  transform * glm::vec4(mesh->verts[tris[t * 3 + 2]], 1.0);
      if (glm::intersectRayTriangle(origin, direction, v0, v1, v2, test_result, _dist)) {
        if (_dist >= 0.0 && _dist < best_dist) {
          ret = i;
          best_dist = _dist;
        }
      }
    }
    i++;
  }
  *dist = best_dist;
  return ret;
}

void gen_uvs(Mesh *mesh) {
//std::vector<glm::vec2> gen_uvs(Mesh *mesh) {
    for (Face &face : mesh->faces) {
        glm::vec3 orig = mesh->verts[face.indices[0]];
        glm::quat rot_mod = glm::rotation(glm::vec3(0.0, 0.0, 1.0), get_normal_face(mesh, &face));
        face.uvs[0] = glm::vec2(0.0, 0.0);
        for (u32 g = 0; g < face.indices.size(); g++) {
            //face->uvs[g] = mesh->verts[face->indices[g]] - orig;
            glm::vec2 vedoe = glm::vec2(glm::rotate(rot_mod, glm::vec4(mesh->verts[face.indices[g]] - orig, 1.0)));
            face.uvs[g] = vedoe;
            //printf("%s\n", glm::to_string(vedoe).c_str());
        }
    }
}

glm::vec3 get_face_center(Mesh *mesh, u32 facei) {
  if (facei >= mesh->faces.size()) {
    return glm::vec3(0.0f);
  }
  glm::vec3 res = glm::vec3(0.0f);
  for (u32 is : mesh->faces[facei].indices) {
    res += mesh->verts[is];
  }
  res /= mesh->faces[facei].indices.size();
  return res;
}

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

void recenter_mesh(Mesh *mesh, glm::vec3 center) {
  for (glm::vec3 &vert : mesh->verts) {
    vert -= center;
  }
}

