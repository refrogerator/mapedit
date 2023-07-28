#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

int gltfGetCount(cgltf_type type) {
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

int gltfTypeToGLType(cgltf_component_type type) {
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

struct Material {
  glm::vec3 albedo;
};

struct GltfMesh {
  u32 elements;
  u32 vao;
  u32 tex;
  Material material;
  glm::mat4 transform;
  char *name;
  AABB *aabb;
};

struct Model {
  GltfMesh *meshes;
  glm::mat4 transform;
  u32 count;
};

GltfMesh glUploadMesh(cgltf_node *node) {
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
      printf("%s\n", texture->name);
      int x, y, channels;
      stbi_uc *texture_data = stbi_load_from_memory((stbi_uc*)((u64)texture->image->buffer_view->buffer->data + texture->image->buffer_view->offset), texture->image->buffer_view->size, &x, &y, &channels, 4);

      u32 tex;
      glGenTextures(1, &tex);

      mesh_.tex = tex;

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
      mesh_.tex = -1;
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
      int count = gltfGetCount(accessor->type);
      int type = gltfTypeToGLType(accessor->component_type);;
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
  return mesh_;
}

Model *gltf_upload_model(cgltf_data *data) {
  struct Model *model = (Model*)malloc(sizeof(Model));
  model->count = 0;
  for (int i = 0; i < data->nodes_count; i++) {
    if (data->nodes[i].mesh) {
      model->count++;
    }
  }
  model->meshes = (GltfMesh*)malloc(sizeof(GltfMesh) * model->count);
  int fart = 0;
  for (int i = 0; i < data->nodes_count; i++) {
    if (data->nodes[i].mesh) {
      model->meshes[fart] = glUploadMesh(&data->nodes[i]);
      fart++;
    }
  }
  model->transform = glm::mat4(1.0f);
  return model;
}

void render_model(Model *model) {
  for (int i = 0; i < model->count; i++) {
    GltfMesh *mesh = &model->meshes[i];
    glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(model->transform * mesh->transform));
    glUniform3fv(4, 1, glm::value_ptr(mesh->material.albedo));
    glUniform1i(5, mesh->tex != -1);
    if (mesh->tex != -1) {
      glBindTexture(GL_TEXTURE_2D, mesh->tex);
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
    glUniform1i(5, mesh->tex != -1);

    if (mesh->tex != -1) {
      glBindTexture(GL_TEXTURE_2D, mesh->tex);
    }

    glBindVertexArray(mesh->vao);
    glDrawElements(GL_TRIANGLES, mesh->elements, GL_UNSIGNED_SHORT, 0);
  }
}
