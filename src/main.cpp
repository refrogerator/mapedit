#define GLM_ENABLE_EXPERIMENTAL
#include "glad.c"

#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_scancode.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_video.h>
#include <glad/glad.h>
#include <glm/ext.hpp>
#include <glm/ext/matrix_projection.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unordered_map>
#include <flecs.h>

#include "cgltf.h"
#include <algorithm>
#include <vector>
#include <filesystem>
#include <print>
using std::println;
using std::print;

#include <format>
#include <dirent.h>
#include <sys/types.h>

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl2.h"

#include "camera.hpp"
#include "common.h"
#include "meshes/editing.hpp"
#include "physics/aabb.hpp"
#include "renderer/gltf.hpp"
#include "meshes/brush.hpp"
#include "renderer/shader.hpp"
#include "scene.hpp"

enum ValueType {};

struct Value {
    ValueType type;
    union {};
};

struct CVar {
    char *description;
};

struct Transform3D {
    glm::vec3 pos;
    glm::vec3 rotation;
    glm::vec3 scale;
    glm::mat4 transform;
};

struct Rendered {
    UploadedMesh *mesh;
    Material material;
    int meshes;
    int visible;
};


void add_brush(std::vector<SceneNode> *scene, Brush brush) {
    SceneNode chud = {};
    chud.type = node_type_brush;
    chud.brush = brush;
    scene->push_back(chud);
}

struct Scene {
    u64 len;
    u64 max;
    std::vector<SceneNode> data;
};

struct MaterialList {
    u64 len;
    u64 max;
    Material **data;
};

struct Viewport {
    u32 width;
    u32 height;
    u32 fbo;
};

enum PointEntityType {
    Light,
};

struct PointEntity {
    PointEntityType type;
    glm::vec3 origin;
    glm::vec3 rotation;
    glm::vec3 scale;
    Mesh *mesh;
    UploadedMesh *uploaded;
    AABB *aabb;
    UploadedMesh *aabb_debug;
};

enum EditMode {
    Vertex,
    Edge,
    Face,
    MeshM,
    ObjectM,
};

enum MoveState {
    None,
    Selecting,
    Selected,
    Moving,
    Deselecting,
};

enum EditTool {
    Select,
};

glm::vec3 new_pos_on_plane(glm::vec3 cur_pos, glm::vec3 plane, glm::vec3 origin, glm::vec3 dir) {
    glm::vec3 res =
        glm::length((cur_pos - origin) * plane) / glm::length(dir * plane) * dir;
    return res;
}

glm::vec3 object_new_pos(glm::vec3 cur_pos, glm::vec3 plane, glm::vec3 origin,
        glm::vec3 dir, u32 grid) {
    float griddiv = 256.0 / pow(2.0f, (float)grid);
    griddiv = 1.0 / griddiv;

    glm::vec3 res =
        glm::length((cur_pos - origin) * plane) / glm::length(dir * plane) * dir;
    glm::vec3 jonathan = glm::vec3(round((origin + res) / griddiv) * griddiv) *
        (glm::vec3(1.0f) - plane);
    // printf("jort\n");
    // printf("%s\n", glm::to_string(res).c_str());
    // printf("%s\n", glm::to_string(jonathan + cur_pos * plane).c_str());
    // printf("%s\n", glm::to_string(plane).c_str());
    // printf("%s\n", glm::to_string(cur_pos * plane).c_str());

    return jonathan + cur_pos * plane;
}

glm::vec3 get_gizmo_plane_(char *name) {
    switch (name[0]) {
        case 'x':
            if (name[1] == 'y') {
                return glm::vec3(1.0, 1.0, 0.0);
            } else {
                return glm::vec3(1.0, 0.0, 0.0);
            }
            break;
        case 'y':
            if (name[1] == 'z') {
                return glm::vec3(0.0, 1.0, 1.0);
            } else {
                return glm::vec3(0.0, 1.0, 0.0);
            }
            break;
        case 'z':
            if (name[1] == 'x') {
                return glm::vec3(1.0, 0.0, 1.0);
            } else {
                return glm::vec3(0.0, 0.0, 1.0);
            }
            break;
    }
    return glm::vec3(0.0, 0.0, 0.0);
}

glm::vec3 get_gizmo_plane(char *name) {
    return glm::vec3(1) - get_gizmo_plane_(name);
}

struct Gizmo {
    glm::vec3 pos;
    glm::vec3 hit_pos;
    Model model;
    int visible;
    u32 moving;
};

struct Window {
    int width;
    int height;
    int mx;
    int my;

    bool rel_mode;
    float sens;

    SDL_Window *window;
};

struct DefaultShader {
    u32 program;
    glm::vec3 albedo;
    bool has_tex;
    bool selected;
    glm::vec3 viewPos;
    u32 tex;
    bool gridt;
    bool indexed;
    std::vector<u32> jortnite;


    void update_uniforms();
};

void DefaultShader::update_uniforms() {
    // glUniform3fv(glGetUniformLocation("albedo"), 
}

using vec3 = glm::vec3;

struct Engine {
    u32 shader;
    u32 solid_shader;
    u32 grid_shader;
    u32 text_shader;
    u32 vao;
    u32 grid_vbo;
    u32 line_vbo;
    u32 tex;

    SDL_GLContext gl_context;

    Camera camera;
    Window window;
    Gizmo gizmo;

    double delta;
    u64 last_delta_uint;
    double frametime;
    double fps;

    i32 grid;
    vec3 move_dir = glm::vec3(1);
    int selected_object_class = -1;
    vec3 ray_world;

    std::vector<Model> models;
    std::vector<Selection> selected_brushes;
    std::vector<SceneNode> scene;
    EditMode edit_mode;
    bool extruded;

    bool wireframe;
    int gizmoind;

    bool quit;
};

void update_matrices(Engine *engine, Camera *cam) {
    engine->camera.rotation = cam::rotation(&engine->camera);

    cam->view = glm::toMat4(engine->camera.rotation) * glm::translate(-engine->camera.origin);
    cam->proj = glm::perspective(glm::radians(engine->camera.fov), (float)engine->window.width / (float)engine->window.height, 0.1f, 100.0f);
}

void render_grid(Engine *engine) {
    glm::mat4 model = glm::mat4(1.0f);

    glUseProgram(engine->grid_shader);
    glEnable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // // glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(model));
    // // cubeb.origin = glm::vec3(glm::vec3(round(camera->origin.x), 0.0,
    // round(camera->origin.z)));
    glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(engine->camera.view));
    glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(engine->camera.proj));
    glUniform3fv(5, 1, glm::value_ptr(engine->camera.origin));

    glBindVertexArray(engine->vao);

    glBindBuffer(GL_ARRAY_BUFFER, engine->grid_vbo);

    glUniform1f(4, pow(2.0f, (float)engine->grid));

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 12 + 8, 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 12 + 8, (void *)12);
    glEnableVertexAttribArray(2);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void render_line(Engine *engine) {
    glm::mat4 model = glm::mat4(1.0f);

    glUseProgram(engine->solid_shader);
    glEnable(GL_CULL_FACE);
    glDisable(GL_BLEND);

    glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(engine->camera.view));
    glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(engine->camera.proj));

    glm::vec3 albedo = glm::vec3(0.0, 1.0, 0.0);

    glUniform3fv(4, 1, glm::value_ptr(albedo));
    glBindBuffer(GL_ARRAY_BUFFER, engine->line_vbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 12, 0);
    glEnableVertexAttribArray(0);
    glDrawArrays(GL_LINES, 0, 2);
}

void render_gizmo(Engine *engine) {
    glUniform1ui(9, 0);
    glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(engine->camera.view));
    glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(engine->camera.proj));
    if (engine->gizmo.moving > Selecting && engine->gizmo.moving < Deselecting) {
        glUseProgram(engine->solid_shader); 
        glm::vec3 gizmo_dir = glm::normalize(engine->gizmo.pos - engine->camera.origin);
        engine->gizmo.model.transform = glm::translate(
                glm::mat4(1.0f), engine->camera.origin + gizmo_dir * glm::vec3(15.0f));
        glClear(GL_DEPTH_BUFFER_BIT);
        if (engine->gizmoind != -1) {
            render_model_jort(&engine->gizmo.model, engine->gizmoind);
        } else {
            render_model(&engine->gizmo.model);
        }
    }
}

void render_scene(Engine *engine) {
    glUseProgram(engine->shader);
    glUniform1ui(9, 0);
    // glUniform1ui(9, -1);
    glUniform1ui(5, 0); // has_tex = 0

    glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(engine->camera.view));
    glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(engine->camera.proj));

    glm::vec3 albedo = glm::vec3(0.0, 1.0, 0.0);
    glUniform3fv(4, 1, glm::value_ptr(albedo));

    glUniform1ui(10, 0);
    glUniform1ui(11, 1);

    glEnable(GL_BLEND);
    glLineWidth(3);
    if (engine->edit_mode == Vertex) {
        // render_brushes_points(brushes, selected_brushes);
    }
    if (engine->wireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDisable(GL_CULL_FACE);
    } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glEnable(GL_CULL_FACE);
    }
    glBindTexture(GL_TEXTURE_2D, engine->tex);

    glUniform1ui(10, 0);

    glUniform4fv(glGetUniformLocation(engine->shader, "overlay"), 1, glm::value_ptr(glm::vec4(0.0)));

    for (u32 i = 0; i < engine->scene.size(); i++) {
        switch (engine->scene[i].type) {
            case node_type_brush: {
                glUseProgram(engine->shader); 
                // printf("brush type\n");
                glUniform1ui(glGetUniformLocation(engine->shader, "has_tex"), 1);
                glUniform1ui(glGetUniformLocation(engine->shader, "indexed"), 1);
                glUniform1ui(glGetUniformLocation(engine->shader, "gridt"), 1);
                // for (int i = 0; i < scene.size(); i++) {
                int elem = 0;
                for (u32 x = 0; x < engine->selected_brushes.size(); x++) {
                    if (engine->selected_brushes[x].obj == i) {
                        elem = 1 + x;
                        break;
                    }
                }

                glm::vec3 albedo = glm::vec3(1.0);
                if (elem) {
                    // glm::vec3 albedo = glm::vec3(1.0, 0.0, 0.0);
                    glUniform3fv(glGetUniformLocation(engine->shader, "albedo"), 1, glm::value_ptr(albedo));

                    if (engine->selected_brushes.size() && engine->selected_brushes[elem - 1].faces.size() && engine->edit_mode == Face) {
                        glUniform1ui(glGetUniformLocation(engine->shader, "jortnite_len"), engine->selected_brushes[elem - 1].faces.size());

                        glUniform1uiv(glGetUniformLocation(engine->shader, "jortnite"), engine->selected_brushes[elem - 1].faces.size(), engine->selected_brushes[elem - 1].faces.data());
                    }
                    //glUniform1ui(9, -1);
                } else {
                    glm::vec3 albedo = glm::vec3(1.0);
                    glUniform3fv(glGetUniformLocation(engine->shader, "albedo"), 1, glm::value_ptr(albedo));
                    glUniform1ui(glGetUniformLocation(engine->shader, "jortnite_len"), 0);
                }

                if (elem) {
                    glm::vec4 overlay = glm::vec4(0.0, 0.5, 1.0, 0.3);

                    glUniform4fv(glGetUniformLocation(engine->shader, "overlay"), 1, glm::value_ptr(overlay));
                }

                render_brush(&engine->scene[i].brush);

                if (engine->edit_mode == Vertex) {
                    render_brush_points(&engine->scene[i].brush);
                }

                glUniform4fv(glGetUniformLocation(engine->shader, "overlay"), 1, glm::value_ptr(glm::vec4(0.0)));

                if (elem) {
                    glUseProgram(engine->solid_shader); 

                    glm::mat4 model = get_brush_matrix(&engine->scene[i].brush);

                    glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(model));
                    glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(engine->camera.view));
                    glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(engine->camera.proj));
                    glUniform3fv(5, 1, glm::value_ptr(engine->camera.origin));

                    albedo = glm::vec3(0.0, 0.5, 1.0);

                    glUniform3fv(glGetUniformLocation(engine->solid_shader, "albedo"), 1, glm::value_ptr(albedo));
                    glBindVertexArray(engine->scene[i].brush.edges.vao);
                    glUniform1ui(6, 1);
                    glDrawArrays(GL_LINES, 0, engine->scene[i].brush.edges.num_verts);
                    glUniform1ui(6, 0);
                }
                break;
            }
            case node_type_object:
                glUseProgram(engine->shader); 
                glUniform1ui(glGetUniformLocation(engine->shader, "has_tex"), 0);
                glUniform1ui(glGetUniformLocation(engine->shader, "indexed"), 0);
                glUniform1ui(glGetUniformLocation(engine->shader, "gridt"), 0);
                engine->scene[i].object.model->transform =
                    glm::translate(glm::mat4(1.0f), engine->scene[i].object.pos);
                render_model(engine->scene[i].object.model);
                break;
        }
    }

}

void render(Engine *engine) {
    glClearColor(0.0, 0.5, 1.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_MULTISAMPLE);

    update_matrices(engine, &engine->camera);

    render_grid(engine);
    render_line(engine);

    render_scene(engine);

    render_gizmo(engine);

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    SDL_GL_SwapWindow(engine->window.window);
}

void gizmo_handle_events(Engine *engine) {
}

void handle_selections(Engine *engine) {
    float dist;
    u32 jorter = select_node(engine->scene, engine->camera.origin, engine->ray_world, &dist);
    enum NodeType jorter_type;
    if (jorter != -1) {
        jorter_type = engine->scene[jorter].type;
    }

    if (engine->selected_object_class != -1) {
        SceneNode node = {};
        node.type = node_type_object;
        Object object = {};
        object.model = &engine->models[engine->selected_object_class];
        if (jorter == -1) {
            object.pos = glm::vec3(0.0);
        } else {
            object.pos = engine->camera.origin + engine->ray_world * dist;
        }
        node.object = object;
        engine->scene.push_back(node);
        engine->selected_object_class = -1;
        return;
    }

    int is_present = 0;
    if (engine->gizmo.moving == Selected) {
        for (int i = 0; i < engine->gizmo.model.count; i++) {
            float diste;
            GltfMesh *meshe = &engine->gizmo.model.meshes[i];
            AABB temp;
            temp.max = engine->gizmo.model.transform * meshe->transform *
                glm::vec4(meshe->aabb->max, 1.0);
            temp.min = engine->gizmo.model.transform * meshe->transform *
                glm::vec4(meshe->aabb->min, 1.0);
            if (intersect_ray_aabb(&temp, engine->camera.origin, engine->ray_world, &diste)) 
            { is_present = 1; }
        }
    }
    u32 idx = 0;

    if (engine->gizmo.moving != Moving) {
        switch (engine->edit_mode) {
            case Vertex: {
                if (~SDL_GetModState() & KMOD_LSHIFT && !is_present && jorter == -1) {
                    engine->selected_brushes.clear();
                }

                // Brush *brush = get_brush(brushes, selected_brushes[0].obj);
                u32 selected_vert = -1;
                u32 selected_brush = -1;
                float shortest_dist = INFINITY;
                for (int i = 0; i < engine->scene.size(); i++) {
                    // Brush *brush = get_brush(brushes, i);
                    if (engine->scene[i].type != node_type_brush) {
                        continue;
                    }

                    Brush *brush = &engine->scene[i].brush;
                    intersect_brush(brush, engine->camera.origin, engine->ray_world, &dist);
                    for (int j = 0; j < brush->mesh->verts.size(); j++) {
                        glm::vec3 vert = brush->mesh->verts[j];
                        glm::vec2 screen_pos = glm::vec2(
                                glm::project(vert, engine->camera.view * get_brush_matrix(brush), engine->camera.proj,
                                    glm::vec4(0, 0, engine->window.width, -engine->window.height)));
                        screen_pos.y += engine->window.height;
                        float cur_dist = glm::length2(glm::vec2(engine->window.mx, engine->window.my) - screen_pos);
                        if (engine->window.mx > screen_pos.x - 50 && engine->window.my > screen_pos.y - 50 &&
                                engine->window.mx < screen_pos.x + 50 && engine->window.my < screen_pos.y + 50 &&
                                cur_dist < shortest_dist &&
                                dist + 0.1 >= glm::length(engine->camera.origin - vert)) {
                            shortest_dist = cur_dist;
                            selected_vert = j;
                            selected_brush = i;
                        }
                    }
                }

                if (selected_vert == (u32)-1) {
                    if (~SDL_GetModState() & KMOD_LSHIFT && !is_present) {
                        engine->selected_brushes.clear();
                        if (jorter == (u32)-1) {
                            engine->gizmo.moving = Deselecting;
                        }
                    }
                    break;
                }

                Brush *brush = &engine->scene[selected_brush].brush;
                glm::vec3 vert = brush->mesh->verts[selected_vert];
                engine->gizmo.pos = vert;
                engine->gizmo.visible = true;

                int brush_elem = -1;
                int vert_elem = 0;
                for (u32 i = 0; i < engine->selected_brushes.size(); i++) {
                    if (engine->selected_brushes[i].obj == selected_brush) {
                        brush_elem = i;
                        for (u32 j = 0; j < engine->selected_brushes[i].verts.size(); j++) {
                            if (engine->selected_brushes[i].verts[j] == selected_vert) {
                                vert_elem = 1;
                                break;
                            }
                        }
                        break;
                    }
                }

                if (selected_brush != (u32)-1 && !is_present) {
                    if (brush_elem == -1) {
                        engine->selected_brushes.push_back(Selection{(u32)selected_brush});
                        brush_elem = engine->selected_brushes.size() - 1;
                    }
                    if (brush_elem != -1 && !vert_elem) {
                        engine->selected_brushes[brush_elem].verts.push_back(selected_vert);
                    }
                    for (u32 i : engine->selected_brushes[brush_elem].verts) {
                        // TODO somehow this prints INT_MAX sometimes
                        println("{}", i);
                    }
                }
            } break;
        case ObjectM: {
            if (~SDL_GetModState() & KMOD_LSHIFT && !is_present) {
                engine->selected_brushes.clear();
                if (jorter == -1) {
                    engine->gizmo.moving = Deselecting;
                }
            }

            int elem = 0;
            for (u32 i = 0; i < engine->selected_brushes.size(); i++) {
                if (engine->selected_brushes[i].obj == jorter) {
                    elem = 1;
                    idx = i;
                }
            }
            if (!elem) {
                if (jorter != -1 && !is_present) {
                    idx = engine->selected_brushes.size();
                    engine->selected_brushes.push_back(Selection{jorter});
                }
            }

            // if (jorter != -1) {
            //   if (engine->scene[engine->selected_brushes[0].obj].type == node_type_brush) {
            //     init_jort = engine->scene[engine->selected_brushes[0].obj].brush.origin;
            //   } else {
            //     init_jort = engine->scene[engine->selected_brushes[0].obj].object.pos;
            //   }
            // }

            if (engine->selected_brushes.size()) {
                if (jorter_type == node_type_brush) {
                    engine->gizmo.pos = engine->scene[engine->selected_brushes[0].obj].brush.origin;
                } else {
                    engine->gizmo.pos = engine->scene[engine->selected_brushes[0].obj].object.pos;
                }
            }

            break;
        }
        case Face: {
            if (jorter_type != node_type_brush)
                break;

            if (~SDL_GetModState() & KMOD_LSHIFT && !is_present) {
                engine->selected_brushes.clear();
                if (jorter == (u32)-1) {
                    engine->gizmo.moving = Deselecting;
                }
            }

            int elem = 0;
            for (u32 i = 0; i < engine->selected_brushes.size(); i++) {
                if (engine->selected_brushes[i].obj == jorter) {
                    elem = 1;
                    idx = i;
                }
            }
            if (!elem) {
                if (jorter != (u32)-1 && !is_present) {
                    idx = engine->selected_brushes.size();
                    engine->selected_brushes.push_back(Selection{jorter});
                }
            }

            if (engine->selected_brushes.size()) {
                i32 selected_face = -1;
                for (u32 i = 0; i < engine->selected_brushes.size(); i++) {
                    Brush *jortware = &engine->scene[engine->selected_brushes[i].obj].brush;
                    selected_face =
                        select_brush_face(jortware, engine->camera.origin, engine->ray_world);
                    if (selected_face != -1) {
                        break;
                    }
                }
                int face_elem = 0;
                for (i32 i : engine->selected_brushes[idx].faces) {
                    if (selected_face == i) {
                        face_elem = 1;
                    }
                }
                if (~SDL_GetModState() & KMOD_LSHIFT && !face_elem &&
                        !is_present) {
                    engine->selected_brushes[idx].faces.clear();
                }
                if (!face_elem && !is_present) {
                    engine->selected_brushes[idx].faces.push_back(selected_face);
                    Brush *new_brush =
                        &engine->scene[engine->selected_brushes[idx].obj].brush;
                    engine->gizmo.pos = get_face_center(new_brush->mesh,
                            engine->selected_brushes[idx].faces[0]) +
                        new_brush->origin;
                    // printf("%s\n", glm::to_string(engine->gizmo.pos).c_str());
                }
                for (u32 i : engine->selected_brushes[idx].faces) {
                    println("{}", i);
                }
            }
            break;
       }
        }
    }
    engine->gizmo.hit_pos = object_new_pos(engine->gizmo.pos, engine->move_dir,
            engine->camera.origin, engine->ray_world, engine->grid);
    if (engine->gizmo.moving == None && engine->selected_brushes.size()) {
        engine->gizmo.moving = Selecting;
    }
    if (engine->selected_brushes.size() && is_present) {
        if (engine->gizmo.moving != None && engine->gizmo.moving != Deselecting) {
            engine->gizmo.moving = Moving;
        }
    }
}

void handle_events(Engine *engine) {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL2_ProcessEvent(&event);

        switch (event.type) {
            case SDL_QUIT:
                engine->quit = 1;
                break;
            case SDL_WINDOWEVENT:
                switch (event.window.event) {
                    case SDL_WINDOWEVENT_RESIZED:
                        glViewport(0, 0, event.window.data1, event.window.data2);
                        engine->window.width = event.window.data1;
                        engine->window.height = event.window.data2;
                }
                break;
            case SDL_MOUSEMOTION:
                // engine->camera.rotation =
                // glm::normalize(glm::quat(glm::vec3(event.motion.yrel * sens,
                // event.motion.xrel * sens, 0.0f)) + engine->camera.rotation);
                if (engine->window.rel_mode) {
                    engine->camera.yaw += event.motion.xrel * engine->window.sens;
                    engine->camera.pitch += event.motion.yrel * engine->window.sens;
                    if (engine->camera.pitch > 1.5f) {
                        engine->camera.pitch = 1.5f;
                    } else if (engine->camera.pitch < -1.5f) {
                        engine->camera.pitch = -1.5f;
                    }
                    // printf("%f, %f\n", engine->camera.yaw, engine->camera.pitch);
                }
                engine->window.mx = event.motion.x;
                engine->window.my = event.motion.y;
                break;
            case SDL_MOUSEBUTTONDOWN:
                switch (event.button.button) {
                    case SDL_BUTTON_RIGHT:
                        engine->window.rel_mode = true;
                        SDL_SetRelativeMouseMode(SDL_TRUE);
                        break;
                    case SDL_BUTTON_LEFT:
                        // glm::vec3 temp = glm::vec3(glm::vec4(0.0f, 0.0f, -1.0f, 1.0f) *
                        // engine->camera.rotation); temp = glm::normalize(temp);
                        handle_selections(engine);
                        break;
                }
                break;
            case SDL_MOUSEBUTTONUP:
                switch (event.button.button) {
                    case SDL_BUTTON_RIGHT:
                        engine->window.rel_mode = false;
                        SDL_SetRelativeMouseMode(SDL_FALSE);
                        break;
                    case SDL_BUTTON_LEFT:
                        if (engine->gizmo.moving == Deselecting) {
                            engine->gizmo.moving = None;
                        } else if (engine->gizmo.moving != None) {
                            engine->gizmo.moving = Selected;
                        }
                        engine->extruded = 0;
                        break;
                }
                break;
            case SDL_MOUSEWHEEL: {
                                     // if (selected_brush == -1)
                                     //	 break;
                                     // i32 dir;
                                     // if (event.wheel.y < 0) {
                                     //	 dir = 1;
                                     // } else {
                                     //	 dir = -1;
                                     // }
                                     // if (SDL_GetModState() & KMOD_LSHIFT) {
                                     //	 extrude_face(brushes.data[selected_brush].mesh, selected_face);
                                     // }
                                     // move_face_along_normal(brushes.data[selected_brush].mesh,
                                     // selected_face, dir, grid); brushes.data[selected_brush] =
                                     // *update_brush(&brushes.data[selected_brush]); break;
                                 }
            case SDL_KEYDOWN:
                switch (event.key.keysym.scancode) {
                    case SDL_SCANCODE_E:
                        engine->grid -= 1;
                        break;
                    case SDL_SCANCODE_Q:
                        // if (grid != 0) {
                        engine->grid += 1;
                        // }
                        break;
                    case SDL_SCANCODE_B:
                        add_brush(&engine->scene, *new_brush());
                        break;
                    case SDL_SCANCODE_G:
                        break;
                    case SDL_SCANCODE_1:
                        engine->edit_mode = Vertex;
                        break;
                    case SDL_SCANCODE_3:
                        engine->edit_mode = Face;
                        break;
                    case SDL_SCANCODE_5:
                        engine->edit_mode = ObjectM;
                        break;
                    case SDL_SCANCODE_EQUALS:
                        engine->wireframe = !engine->wireframe;
                        break;
                    case SDL_SCANCODE_R:
                        for (Selection i : engine->selected_brushes) {
                            recenter_brush(&engine->scene[i.obj].brush);
                        }
                        break;
                    case SDL_SCANCODE_C:
                        for (Selection i : engine->selected_brushes) {
                            remove_node(&engine->scene, i.obj);
                        }
                        engine->selected_brushes.clear();
                        engine->gizmo.visible = false;
                        engine->gizmo.moving = None;
                        break;
                }
                break;
        }
    }

    // handle movement
    float speed = 10.0;
    const Uint8 *kb = SDL_GetKeyboardState(NULL);
    if (kb[SDL_SCANCODE_LSHIFT]) {
        speed = 40.0;
    }
    if (kb[SDL_SCANCODE_S]) {
        cam::translate_z(&engine->camera, -speed * engine->delta);
    }
    if (kb[SDL_SCANCODE_W]) {
        cam::translate_z(&engine->camera, speed * engine->delta);
    }
    if (kb[SDL_SCANCODE_D]) {
        cam::translate_x(&engine->camera, -speed * engine->delta);
    }
    if (kb[SDL_SCANCODE_A]) {
        cam::translate_x(&engine->camera, speed * engine->delta);
    }
    if (kb[SDL_SCANCODE_SPACE]) {
        cam::translate_y(&engine->camera, speed * engine->delta);
    }
    // if (kb[SDL_SCANCODE_LALT]) {
    //     vertical = 1;
    // } else {
    //     vertical = 0;
    // }
}

void calculate_ray_world(Engine *engine) {
    float ndx = (2.0 * engine->window.mx) / engine->window.width - 1.0;
    float ndy = 1.0 - (2.0 * engine->window.my) / engine->window.height;
    glm::vec4 clip = glm::vec4(ndx, ndy, -1.0, 1.0);

    glm::vec4 ray_eye = glm::inverse(engine->camera.proj) * clip;
    ray_eye = glm::vec4(ray_eye.x, ray_eye.y, -1.0, 0.0);

    glm::vec3 ray_world_ = glm::vec3(glm::inverse(engine->camera.view) * ray_eye);
    engine->ray_world = glm::normalize(ray_world_);
}

void init_window(Engine *engine) {
    Window *window = &engine->window;

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER |
            SDL_INIT_SENSOR);

    window->window = SDL_CreateWindow(
            "fortnite moment", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1920,
            1080, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    SDL_JoystickEventState(SDL_ENABLE);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

    engine->gl_context = SDL_GL_CreateContext(window->window);
    gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress);

    window->width = 1920;
    window->height = 1080;
    glViewport(0, 0, 1920, 1080);
    SDL_GetMouseState(&engine->window.mx, &engine->window.my);
        
    window->sens = 0.004;
}

void init_renderer(Engine *engine) {
    u32 vert_shader = loadShader("fortnite.vert", GL_VERTEX_SHADER);
    u32 frag_shader = loadShader("fortnite.frag", GL_FRAGMENT_SHADER);
    u32 grid_fshader = loadShader("grid.frag", GL_FRAGMENT_SHADER);
    u32 solid_fshader = loadShader("solid_color.frag", GL_FRAGMENT_SHADER);

    u32 textv = loadShader("text.vert", GL_VERTEX_SHADER);
    u32 textf = loadShader("text.frag", GL_FRAGMENT_SHADER);

    engine->shader = createProgram(vert_shader, frag_shader, false);
    engine->solid_shader = createProgram(vert_shader, solid_fshader, false);
    engine->grid_shader = createProgram(vert_shader, grid_fshader, true);
    engine->text_shader = createProgram(textv, textf, true);

    float vertices[] = {
        50.0,  0.0, -50.0, 1.0, 0.0,   -50.0, 0.0, -50.0,
        0.0,   0.0, -50.0, 0.0, 50.0,  0.0,   1.0,

        -50.0, 0.0, 50.0,  0.0, 1.0,   50.0,  0.0, 50.0,
        1.0,   1.0, 50.0,  0.0, -50.0, 1.0,   0.0,
    };

    glGenBuffers(1, &engine->grid_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, engine->grid_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glGenVertexArrays(1, &engine->vao);

    float lines[] = {0.0, -100.0, 0.0, 0.0, 100.0, 0.0};
    glGenBuffers(1, &engine->line_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, engine->line_vbo);
    glBufferData(GL_ARRAY_BUFFER, 2 * 3 * sizeof(float), lines, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);


    int x, y, channels;
    stbi_uc *texture_data =
        stbi_load("textures/cobble.png", &x, &y, &channels, 0);

    SDL_GL_SetSwapInterval(1);

    glGenTextures(1, &engine->tex);

    glBindTexture(GL_TEXTURE_2D, engine->tex);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, 16.0f);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
            GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE,
            texture_data);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(texture_data);
}

void init_camera(Camera *cam) {
    cam->origin = glm::vec3(3.0f, 3.0f, 3.0f);
    cam->fov = 90.0f;
    cam->pitch = 0.5f;
    cam->yaw = glm::radians(-45.0f);
    // cam->rotation = glm::normalize(glm::quat());
}

void init_gizmo(Engine *engine) {
    cgltf_options options{};
    cgltf_data *data = NULL;
    cgltf_result result = cgltf_parse_file(&options, "res/models/tgizmo.glb", &data);
    cgltf_load_buffers(&options, data, "./");

    engine->gizmo.pos = glm::vec3(0.0f);
    engine->gizmo.hit_pos = glm::vec3(0.0f);
    Model model = gltf_upload_model(data);
    engine->gizmo.model = model;
    engine->gizmo.visible = 1;

    cgltf_free(data);
}

void load_models(Engine *engine) {
    for (const auto &entry : std::filesystem::directory_iterator("./models")) {
        if (entry.path().extension() == ".glb") {
            // println("{}", entry.path().extension().string());
            cgltf_options options{};
            cgltf_data *data = NULL;
            cgltf_result result = cgltf_parse_file(&options, entry.path().c_str(), &data);
            if (result != cgltf_result_success) {
                continue;
            }

            cgltf_load_buffers(&options, data, "./models/");

            Model model = gltf_upload_model(data);

            model.name = entry.path().filename();

            // flecs::entity ent = world.entity(name).set(model);
            engine->models.push_back(model);

            // cgltf_free(data);
        }
    }

    for (Model &model : engine->models) {
        println("g{}", model.name);
    }
}

void init_ui(Engine *engine) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |=
        ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |=
        ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls
                                           // io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

                                           // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(engine->window.window, engine->gl_context);
    ImGui_ImplOpenGL3_Init();
}

void init_engine(Engine *engine) {
    engine->grid = 7;
    engine->edit_mode = ObjectM;

    init_window(engine);
    init_renderer(engine);
    init_ui(engine);
    load_models(engine);
    init_camera(&engine->camera);
    init_gizmo(engine);
}

void handle_gizmo_movement(Engine *engine) {
    float diste;
    float best_dist = INFINITY;
    if (engine->gizmo.moving == Selected) {
        engine->gizmoind = -1;
        for (int i = 0; i < engine->gizmo.model.count; i++) {
            GltfMesh *meshe = &engine->gizmo.model.meshes[i];
            AABB temp;
            temp.max = engine->gizmo.model.transform * meshe->transform *
                glm::vec4(meshe->aabb->max, 1.0);
            temp.min = engine->gizmo.model.transform * meshe->transform *
                glm::vec4(meshe->aabb->min, 1.0);
            if (intersect_ray_aabb(&temp, engine->camera.origin, engine->ray_world, &diste) &&
                    diste < best_dist) {
                engine->gizmoind = i;
                best_dist = diste;
            }
        }
        if (engine->gizmoind != -1) {
            engine->move_dir = get_gizmo_plane(engine->gizmo.model.meshes[engine->gizmoind].name);
        } else {
            engine->move_dir = glm::vec3(1);
        }
    }

    if (engine->gizmo.moving == Moving) {
        glm::vec3 init_pos = engine->gizmo.pos;

        glm::vec3 diff = engine->gizmo.pos - engine->gizmo.hit_pos;
        engine->gizmo.hit_pos = object_new_pos(engine->gizmo.hit_pos, engine->move_dir,
                engine->camera.origin, engine->ray_world, engine->grid);
        engine->gizmo.pos = engine->gizmo.hit_pos + diff;

        glm::vec3 offs = engine->gizmo.pos - init_pos;

        if (engine->selected_brushes.size() && offs != glm::vec3(0.0)) {
            // printf("jort\n");
            switch (engine->edit_mode) {
                case ObjectM: {
                    Brush *brusher = &engine->scene[engine->selected_brushes[0].obj].brush;
                    // puts(glm::to_string(gizmo.pos).c_str());

                    brusher->origin = engine->gizmo.pos;
                    for (int i = 0; i < engine->selected_brushes.size(); i++) {
                        if (i == 0) {
                            continue;
                        }
                        Brush *brush = &engine->scene[engine->selected_brushes[i].obj].brush;
                        brush->origin += offs;
                    }
                    // init_jort = brusher->origin;
                    break;
                }
                case Face: {
                    // if (offs != glm::vec3(0.0, 0.0, 0.0)) {
                    // printf("%s\n", glm::to_string(offs).c_str());
                    // }
                    for (Selection selection : engine->selected_brushes) {
                        Brush *cur_brush = &engine->scene[selection.obj].brush;
                        if (SDL_GetModState() & KMOD_LSHIFT && !engine->extruded) {
                            for (u32 cur_face : selection.faces) {
                                println("extruded\n");
                                extrude_face(cur_brush->mesh, cur_face);
                            }
                            engine->extruded = 1;
                        }
                        move_faces(cur_brush->mesh, selection.faces, offs);
                        update_brush(cur_brush);
                    }
                    break;
                }
                case Vertex: {
                    for (Selection selection : engine->selected_brushes) {
                        Brush *cur_brush = &engine->scene[selection.obj].brush;
                        for (u32 vert : selection.verts) {
                            cur_brush->mesh->verts[vert] += offs;
                        }
                        update_brush(cur_brush);
                    }
                }
            }
        }
    }
}

void handle_ui(Engine *engine) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    if (engine->selected_object_class != -1) {
        ImGui::Text("%s", engine->models[engine->selected_object_class].name.c_str());
    } else {
        ImGui::Text("");
    }

    ImVec2 button_sz(100, 100);
    ImGuiStyle &style = ImGui::GetStyle();
    float window_visible_x2 =
        ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
    for (int i = 0; i < engine->models.size(); i++) {
        if (ImGui::Button(engine->models[i].name.c_str(), button_sz)) {
            engine->selected_object_class = i;
        }
        float last_button_x2 = ImGui::GetItemRectMax().x;
        float next_button_x2 =
            last_button_x2 + style.ItemSpacing.x +
            button_sz.x; // Expected position if next button was on same line
        if (i + 1 < engine->models.size() && next_button_x2 < window_visible_x2)
            ImGui::SameLine();
    }
}

void update_delta_time(Engine *engine) {
    u64 delta_uint = SDL_GetPerformanceCounter() - engine->last_delta_uint;
    engine->last_delta_uint = SDL_GetPerformanceCounter();
    engine->delta = (double)delta_uint / (double)SDL_GetPerformanceFrequency();
}

int main() {
    Engine _engine = {};
    Engine *engine = &_engine;

    init_engine(engine);

    double frametime_total = 0;
    double frametime = 0.0;
    double fps = 0.0;

    int frames = 0;

    Brush *brush = new_brush();
    add_brush(&engine->scene, *brush);

    while (!engine->quit) {
        update_delta_time(engine);
        if (frametime_total > 0.1) {
            frametime = frametime_total / (double)frames;
            frametime_total = 0;
            fps = 1.0 / frametime;
            frames = 0;
        }
        frametime_total += engine->delta;

        calculate_ray_world(engine);
        handle_events(engine);
        handle_ui(engine);
        handle_gizmo_movement(engine);
        render(engine);
        frames++;
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyWindow(engine->window.window);
    SDL_Quit();
}
