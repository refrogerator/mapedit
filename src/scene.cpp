u32 select_node(std::vector<SceneNode> scene, glm::vec3 origin, glm::vec3 dir, float *_dist) {
  float dist = INFINITY;
  float best_dist = INFINITY;
  u32 ret = -1;
  for (int i = 0; i < scene.size(); i++) {
    switch (scene[i].type) {
      case node_type_brush:
        if (!intersect_brush(&scene[i].brush, origin, dir, &dist)) {
          continue;
        }
        break;
      case node_type_object:
        if (!gltf_intersect_model(scene[i].object.model, origin, dir, &dist)) {
          continue;
        }
        break;
      default:
        continue;
    }
    if (dist >= 0.0 && dist < best_dist) {
      best_dist = dist;
      ret = i;
    }
  }
  *_dist = best_dist;
  return ret;
}

void remove_node(std::vector<SceneNode> *scene, int idx) {
  scene->erase(scene->begin() + idx);
}
