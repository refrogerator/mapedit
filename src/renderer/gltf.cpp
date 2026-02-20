#define STB_IMAGE_IMPLEMENTATION
#include "gltf.hpp"

int gltf_get_count(cgltf_type type) {
  switch (type) {
    case cgltf_type_vec3:
      return 3;
    case cgltf_type_vec2:
      return 2;
    case cgltf_type_scalar:
      return 1;
  }
  return 0;
}

int gltf_type_to_gl_type(cgltf_component_type type) {
  switch (type) {
  	case cgltf_component_type_r_8:
      return GL_BYTE;
  	case cgltf_component_type_r_8u:
      return GL_UNSIGNED_BYTE;
  	case cgltf_component_type_r_16:
      return GL_SHORT;
  	case cgltf_component_type_r_16u:
      return GL_UNSIGNED_SHORT;
  	case cgltf_component_type_r_32u:
      return GL_UNSIGNED_INT;
  	case cgltf_component_type_r_32f:
      return GL_FLOAT;
  }
  return 0;
}

int gltf_intersect_mesh(GltfMesh *mesh, glm::mat4 transform, glm::vec3 origin, glm::vec3 direction, float *dist) {
  AABB temp;
  temp.max = transform * mesh->transform *
             glm::vec4(mesh->aabb->max, 1.0);
  temp.min = transform * mesh->transform *
             glm::vec4(mesh->aabb->min, 1.0);
  return intersect_ray_aabb(&temp, origin, direction, dist);
}

// int gltf_intersect_model(Model *model, glm::vec3 origin, glm::vec3 direction, float *dist) {
//   int joe = 0;
//   for (int i = 0; i < model->count; i++) {
//     if (gltf_intersect_mesh(&model->meshes[i], model->transform, origin, direction, dist)) {
//       joe = 1;
//     }
//   }
//   return joe;
// }

GltfMesh gltf_upload_mesh(cgltf_node *node) {
  cgltf_mesh *mesh = node->mesh;
  printf("%s\n", node->name);
  printf("meshname: %s, primitives:%lu\n", mesh->name, mesh->primitives_count);
  u32 vao;
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  struct GltfMesh mesh_;
  mesh_.vao = vao;
  mesh_.name = (char*)malloc(strlen(node->name) + 1);
  strcpy(mesh_.name, node->name);

  glm::vec3 translation;
    translation.x = node->translation[0];
    translation.y = node->translation[1];
    translation.z = node->translation[2];
  glm::vec3 scalev;
    scalev.x = node->scale[0];
    scalev.y = node->scale[1];
    scalev.z = node->scale[2];
  glm::quat rotationq;
    rotationq.x = node->rotation[0];
    rotationq.y = node->rotation[1];
    rotationq.z = node->rotation[2];
    rotationq.w = node->rotation[3];
  glm::mat4 transform = glm::translate(glm::mat4(1.0f), translation);
  glm::mat4 rotation = glm::toMat4(rotationq);
  glm::mat4 scale = glm::scale(glm::mat4(1.0f), scalev);

  mesh_.transform = transform * rotation * scale;

  for (int i = 0; i < mesh->primitives_count; i++) {
    cgltf_primitive *primitive = &mesh->primitives[i];
    cgltf_accessor *indices = primitive->indices;

    mesh_.material.albedo.x = primitive->material->pbr_metallic_roughness.base_color_factor[0];
    mesh_.material.albedo.y = primitive->material->pbr_metallic_roughness.base_color_factor[1];
    mesh_.material.albedo.z = primitive->material->pbr_metallic_roughness.base_color_factor[2];

    cgltf_texture *texture = primitive->material->pbr_metallic_roughness.base_color_texture.texture;
    if (texture) {
      // printf("%s\n", texture->name);
      int x, y, channels;
      stbi_uc *texture_data = stbi_load_from_memory((stbi_uc*)((u64)texture->image->buffer_view->buffer->data + texture->image->buffer_view->offset), texture->image->buffer_view->size, &x, &y, &channels, 4);

      u32 tex;
      glGenTextures(1, &tex);

      mesh_.material.tex = tex;

      glBindTexture(GL_TEXTURE_2D, tex);

      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, 16.0f);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture_data);
      glGenerateMipmap(GL_TEXTURE_2D);

      stbi_image_free(texture_data);
    } else {
      mesh_.material.tex = -1;
    }

    u32 ebo;
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices->buffer_view->size, (void*)((u64)indices->buffer_view->buffer->data + indices->buffer_view->offset), GL_STATIC_DRAW);
    mesh_.elements = indices->count;

    u32 vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, primitive->attributes->data->buffer_view->buffer->size, primitive->attributes->data->buffer_view->buffer->data, GL_STATIC_DRAW);

    for (int a = 0; a < primitive->attributes_count; a++) {
      cgltf_attribute *attr = &mesh->primitives[i].attributes[a];
      cgltf_accessor *accessor = attr->data;
      int count = gltf_get_count(accessor->type);
      int type = gltf_type_to_gl_type(accessor->component_type);;
      int attribute = -1;
      switch (attr->type) {
        case cgltf_attribute_type_position:
          {
            attribute = 0;
            glm::vec3 max = glm::vec3(-INFINITY, -INFINITY, -INFINITY);
            glm::vec3 min = glm::vec3(INFINITY, INFINITY, INFINITY);
            float *maxp = glm::value_ptr(max);
            float *minp = glm::value_ptr(min);
            AABB *aabb = (AABB*)malloc(sizeof(AABB));
            for (int verti = 0; verti < accessor->count; verti++) {
              float *vert = (float*)(((u64)primitive->attributes->data->buffer_view->buffer->data + (u64)accessor->buffer_view->offset + (u64)accessor->offset) + (u64)accessor->stride * (u64)verti);
              for (int jortes = 0; jortes < 3; jortes++) {
                if (vert[jortes] > maxp[jortes]) {
                  maxp[jortes] = vert[jortes];
                } else if (vert[jortes] < minp[jortes]) {
                  minp[jortes] = vert[jortes];
                }
              }
            }
            aabb->max = max;
            aabb->min = min;
            mesh_.aabb = aabb;
          }
          break;
        case cgltf_attribute_type_normal:
          attribute = 1;
          break;
        case cgltf_attribute_type_texcoord:
          attribute = 2;
          break;
        default:
          break;
      }
      if (attribute == -1) {
        continue;
      }
      glVertexAttribPointer(attribute, count, type, GL_FALSE, accessor->stride, (void*)(accessor->buffer_view->offset + accessor->offset));
      glEnableVertexAttribArray(attribute);
    }
  }
  // mesh_.aabb = get_aabb(mesh_);
  return mesh_;
}

Model gltf_upload_model(cgltf_data *data) {
  struct Model model = {0};
  model.data = data;
  model.count = 0;
  for (int i = 0; i < data->nodes_count; i++) {
    if (data->nodes[i].mesh) {
      model.count++;
    }
  }
  model.meshes = (GltfMesh*)malloc(sizeof(GltfMesh) * model.count);
  int fart = 0;
  for (int i = 0; i < data->nodes_count; i++) {
    if (data->nodes[i].mesh) {
      model.meshes[fart] = gltf_upload_mesh(&data->nodes[i]);
      model.meshes[fart].mesh = data->nodes[i].mesh;
      fart++;
    }
  }
  model.transform = glm::mat4(1.0f);
  return model;
}

void gltf_mesh_get_physics(GltfMesh *_mesh, Physics *chud) {
  cgltf_mesh *mesh = _mesh->mesh;
  for (int i = 0; i < mesh->primitives_count; i++) {
    cgltf_primitive *primitive = &mesh->primitives[i];
    cgltf_accessor *indices = primitive->indices;
    for (int a = 0; a < primitive->attributes_count; a++) {
      cgltf_attribute *attr = &primitive->attributes[a];
      cgltf_accessor *accessor = attr->data;
      switch (attr->type) {
        case cgltf_attribute_type_position:
          u32 *inds = (u32*)((u64)indices->buffer_view->buffer->data + indices->buffer_view->offset);
          for (int ind = 0; ind < indices->count; ind++) {
            float *vert = (float*)(((u64)primitive->attributes->data->buffer_view->buffer->data + (u64)accessor->buffer_view->offset + (u64)accessor->offset) + (u64)accessor->stride * (u64)inds[ind]);
            Triangle tri;
            tri.v0 = { vert[0], vert[1], vert[2] };
            ind++;
            vert = (float*)(((u64)primitive->attributes->data->buffer_view->buffer->data + (u64)accessor->buffer_view->offset + (u64)accessor->offset) + (u64)accessor->stride * (u64)inds[ind]);
            tri.v1 = { vert[0], vert[1], vert[2] };
            ind++;
            vert = (float*)(((u64)primitive->attributes->data->buffer_view->buffer->data + (u64)accessor->buffer_view->offset + (u64)accessor->offset) + (u64)accessor->stride * (u64)inds[ind]);
            tri.v2 = { vert[0], vert[1], vert[2] };
            chud->tris.push_back(tri);
          }
          break;
      }
    }
  }
}

Physics gltf_model_get_physics(Model *model) {
  Physics phys = {};
  for (int i = 0; i < model->count; i++) {
    gltf_mesh_get_physics(&model->meshes[i], &phys);
  }
  return phys;
}

// int gltf_intersect_mesh(GltfMesh *_mesh, glm::mat4 tr, glm::vec3 origin, glm::vec3 direction, float *_dist) {
//   glm::mat4 transform = _mesh->transform;
//   cgltf_mesh *mesh = _mesh->mesh;
//   int res = 0;
//   float dist = INFINITY;
//   float best_dist = INFINITY;
//   glm::vec2 test_result;
//   for (int i = 0; i < mesh->primitives_count; i++) {
//     cgltf_primitive *primitive = &mesh->primitives[i];
//     for (int a = 0; a < primitive->attributes_count; a++) {
//       cgltf_attribute *attr = &primitive->attributes[a];
//       cgltf_accessor *accessor = attr->data;
//       switch (attr->type) {
//         case cgltf_attribute_type_position:
//           for (int trii = 0; trii < accessor->count; trii++) {
//             glm::vec3 tri[3];
//             for (int verti = 0; verti < 3; verti++) {
//               float *vert = (float*)(((u64)primitive->attributes->data->buffer_view->buffer->data + (u64)accessor->buffer_view->offset + (u64)accessor->offset) + (u64)accessor->stride * ((u64)trii * 3 + verti));
//               // tri[verti].x = vert[0];
//               // tri[verti].y = vert[1];
//               // tri[verti].z = vert[2];
//               tri[verti] = glm::vec4(vert[0], vert[1], vert[2], 1.0);
//             }
//             if (glm::intersectRayTriangle(origin, direction, tri[0], tri[1], tri[2], test_result, dist)) {
//               res = 1;
//               if (dist >= 0.0 && dist < best_dist) {
//                 best_dist = dist;
//               }
//             }
//           }
//           break;
//       }
//     }
//   }
//   *_dist = best_dist;
//   return 1;
// }

int gltf_intersect_model(Model *model, glm::vec3 origin, glm::vec3 direction, float *_dist) {
  int res = 0;
  float dist = 0;
  float best_dist = INFINITY;
  for (int i = 0; i < model->count; i++) {
    if (gltf_intersect_mesh(&model->meshes[i], model->transform, origin, direction, &dist)) {
      if (dist < best_dist) {
        best_dist = dist;
      }
      res = 1;
    }
  }
  *_dist = best_dist;
  return res;
}

void render_model(Model *model) {
  glm::mat4 chud = glm::mat4(1.0f);
  for (int i = 0; i < model->count; i++) {
    GltfMesh *mesh = &model->meshes[i];
    glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(model->transform * mesh->transform));
    glUniform3fv(4, 1, glm::value_ptr(mesh->material.albedo));
    glUniform1i(5, mesh->material.tex != -1);
    if (mesh->material.tex != -1) {
      glBindTexture(GL_TEXTURE_2D, mesh->material.tex);
    }
    glBindVertexArray(mesh->vao);
    glDrawElements(GL_TRIANGLES, mesh->elements, GL_UNSIGNED_SHORT, 0);
  }
}

void render_model_jort(Model *model, int index) {
  for (int i = 0; i < model->count; i++) {
    GltfMesh *mesh = &model->meshes[i];
    glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(model->transform * mesh->transform));
    if (i != index) {
      glUniform3fv(4, 1, glm::value_ptr(mesh->material.albedo));
    } else {
      glUniform3fv(4, 1, glm::value_ptr(mesh->material.albedo * glm::vec3(1.4f)));
    }
    glUniform1i(5, mesh->material.tex != -1);

    if (mesh->material.tex != -1) {
      glBindTexture(GL_TEXTURE_2D, mesh->material.tex);
    }

    glBindVertexArray(mesh->vao);
    glDrawElements(GL_TRIANGLES, mesh->elements, GL_UNSIGNED_SHORT, 0);
  }
}
