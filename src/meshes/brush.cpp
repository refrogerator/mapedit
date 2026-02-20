#include "brush.hpp"

void update_mesh(Mesh *mesh, UploadedMesh *umesh) {
  u32 num;
  float *tris = get_tris(mesh, &num);
  u32 vert_size = 3 * sizeof(float) + 3 * sizeof(float) + 2 * sizeof(float) + 1 * sizeof(u32);

  glBindVertexArray(umesh->vao);

  glBindBuffer(GL_ARRAY_BUFFER, umesh->vbo);
  glBufferData(GL_ARRAY_BUFFER, num * 3 * vert_size, tris, GL_STATIC_DRAW);

  free(tris);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, vert_size, 0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, vert_size, (void*)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, vert_size, (void*)(6 * sizeof(float)));
  glEnableVertexAttribArray(2);
  glVertexAttribIPointer(3, 1, GL_UNSIGNED_INT, vert_size, (void*)(8 * sizeof(float)));
  glEnableVertexAttribArray(3);

  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  umesh->num_verts = num * 3;
}

UploadedMesh *upload_mesh(Mesh *mesh) {
  UploadedMesh *umesh = (UploadedMesh*)malloc(sizeof(UploadedMesh));

  u32 vao;
  glGenVertexArrays(1, &vao);

  u32 vbo;
  glGenBuffers(1, &vbo);

  umesh->vao = vao;
  umesh->vbo = vbo;
  umesh->visible = 1;

  update_mesh(mesh, umesh);

  return umesh;
}

void render_mesh(UploadedMesh *mesh) {
  glBindVertexArray(mesh->vao);
  glDrawArrays(GL_TRIANGLES, 0, mesh->num_verts);
}

Brush *new_brush() {
  Brush *brush = (Brush*)malloc(sizeof(Brush));
  brush->origin = glm::vec3(0.0);
  brush->rotation = glm::vec3(0.0);
  brush->scale = glm::vec3(1.0);
  brush->mesh = get_cube();
  brush->uploaded = upload_mesh(brush->mesh);
  brush->aabb = get_aabb(brush->mesh);
  brush->mat = 0;
  return brush;
}

glm::mat4 get_brush_matrix(Brush *brush) {
  glm::mat4 model = glm::mat4(1.0f);
  model = glm::translate(model, brush->origin);
  model = glm::rotate(model, brush->rotation.x, glm::vec3(1.0, 0.0, 0.0));
  model = glm::rotate(model, brush->rotation.y, glm::vec3(0.0, 1.0, 0.0));
  model = glm::rotate(model, brush->rotation.z, glm::vec3(0.0, 0.0, 1.0));
  model = glm::scale(model, brush->scale);
  return model;
}


void render_brush(Brush *brush) {
  glm::mat4 model = get_brush_matrix(brush);
  glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(model));
  render_mesh(brush->uploaded);
}

u32 select_brush_face(Brush *brush, glm::vec3 origin, glm::vec3 dir) {
  glm::mat4 model = get_brush_matrix(brush);
  float dist;
  u32 james = intersect_faces(brush->mesh, model, origin, dir, &dist);
  return james;
}

u32 intersect_brush(Brush *brush, glm::vec3 origin, glm::vec3 dir, float *dist) {
  glm::mat4 model = get_brush_matrix(brush);
  AABB temp;
  temp.max = model * glm::vec4(brush->aabb->max, 1.0);
  temp.min = model * glm::vec4(brush->aabb->min, 1.0);
  if (intersect_ray_aabb(&temp, origin, dir, dist)) {
    if (intersect_faces(brush->mesh, model, origin, dir, dist) != -1) {
      return 1;
    }
  }
  return 0;
}

BrushList new_brushlist() {
  BrushList brushes;
  brushes.len = 0;
  brushes.max = 10;
  brushes.data = (Brush*)malloc(sizeof(Brush) * brushes.max);
  return brushes;
}

BrushList remove_brush(BrushList brushes, u32 index) {
  if (index == -1 || index > brushes.len) {
      return brushes;
  }
  if (brushes.len == brushes.max - 11) {
    brushes.max -= 10;
    brushes.data = (Brush*)realloc(brushes.data, brushes.max * sizeof(Brush));
  }
  brushes.data[index] = brushes.data[brushes.len];
  brushes.len -= 1;
  return brushes;
}

void render_brushes_points(BrushList brushes, std::vector<Selection> selected) {
  glUniform1ui(5, 0);
  glUniform1ui(10, 0);
  glPolygonMode( GL_FRONT_AND_BACK, GL_POINT );
  glPointSize(10);
  for (int i = 0; i < brushes.len; i++) {
	glm::vec3 albedo = glm::vec3(1.0, 1.0, 1.0);
	glUniform3fv(4, 1, glm::value_ptr(albedo));

    render_brush(&brushes.data[i]);
  }
}

void render_brushes(BrushList brushes, std::vector<Selection> selected) {
  glUniform1ui(5, 1);
  glUniform1ui(10, 1);
  for (int i = 0; i < brushes.len; i++) {
    int elem = 0;
    for (int x = 0; x < selected.size(); x++) {
      if (selected[x].obj == i) {
        elem = 1 + x;
        break;
      }
    }
    if (elem) {
      glm::vec3 albedo = glm::vec3(1.0, 0.0, 0.0);
      glUniform3fv(4, 1, glm::value_ptr(albedo));

	  if (selected.size() && selected[elem - 1].faces.size()) {
		glUniform1ui(9, selected[elem - 1].faces.size());
		glUniform1uiv(12, selected[elem - 1].faces.size(), selected[elem - 1].faces.data());
	  }
      //glUniform1ui(9, -1);
    } else {
      glm::vec3 albedo = glm::vec3(1.0);
      glUniform3fv(4, 1, glm::value_ptr(albedo));
      glUniform1ui(9, 0);
    }

    render_brush(&brushes.data[i]);
  }
  glUniform1ui(5, 0);
}

Brush *get_brush(BrushList brushes, u32 index) {
  if (index >= brushes.len) {
    return 0;
  } else {
    return &brushes.data[index];
  }
}

Brush *get_brush_ptr(BrushList brushes, u32 index) {
  if (index >= brushes.len) {
    return 0;
  } else {
    return &brushes.data[index];
  }
}

void update_brush(Brush *brush) {
  gen_uvs(brush->mesh);
  brush->aabb = get_aabb(brush->mesh);
  glm::vec3 center = get_center(brush->aabb);
  // recenter_mesh(brush->mesh, brush->origin, center);
  // brush->origin = center;
  update_mesh(brush->mesh, brush->uploaded);
}
