#include <SDL2/SDL_events.h>
#include <SDL2/SDL_scancode.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_video.h>
#include <glad/glad.h>
#include "glad.c"
#include <SDL2/SDL.h>
#include <glm/ext/matrix_projection.hpp>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <unordered_map>

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

#include <vector>
#include <algorithm>

#include "common.h"
#include "util.c"
#include "renderer.h"
#include "camera.cpp"
#include "meshes/editing.cpp"
#include "physics/aabb.cpp"
#include "renderer/gltf.cpp"


#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"

enum ValueType {
};

struct Value {
	ValueType type;
	union {
	};
};

struct CVar {
	char *description;
};

struct Selection {
	u32 obj;
	std::vector<u32> faces;
	std::vector<u32> verts;
};

struct Object {
    char *classname;
    Model *model;
};

struct ObjectList {
	u64 len;
	u64 max;
	Object **data;
};

ObjectList new_object_list() {
	ObjectList objects;
	objects.len = 0;
	objects.max = 10;
	objects.data = (Object**)malloc(sizeof(Object*));
	return objects;
}

ObjectList add_object(ObjectList objects, Object *object) {
	if (objects.len == objects.max - 1) {
		objects.max += 10;
		objects.data = (Object**)realloc(objects.data, objects.max * sizeof(Object*));
	}
	objects.data[objects.len] = object;
	objects.len += 1;
	return objects;
}

ObjectList remove_object(ObjectList objects, u32 index) {
	if (objects.len == objects.max - 11) {
		objects.max -= 10;
		objects.data = (Object**)realloc(objects.data, objects.max * sizeof(Object*));
	}
	objects.data[index] = objects.data[objects.len];
	objects.len -= 1;
	return objects;
}

Object *get_object(ObjectList objects, u32 index) {
	if (index >= objects.len) {
		return 0;
	} else {
		return objects.data[index];
	}
}

struct MaterialList {
	u64 len;
	u64 max;
	Material **data;
};

MaterialList new_materiallist() {
    MaterialList materials;
    materials.len = 0;
    materials.max = 10;
    materials.data = (Material**)malloc(sizeof(Material*));
    return materials;
}

MaterialList add_material(MaterialList materials, Material *material) {
	if (materials.len == materials.max - 1) {
		materials.max += 10;
		materials.data = (Material**)realloc(materials.data, materials.max * sizeof(Material*));
	}
	materials.data[materials.len] = material;
	materials.len += 1;
	return materials;
}

MaterialList remove_material(MaterialList materials, u32 index) {
	if (materials.len == materials.max - 11) {
		materials.max -= 10;
		materials.data = (Material**)realloc(materials.data, materials.max * sizeof(Material*));
	}
	materials.data[index] = materials.data[materials.len];
	materials.len -= 1;
	return materials;
}

Material *get_material(MaterialList materials, u32 index) {
	if (index >= materials.len) {
		return 0;
	} else {
		return materials.data[index];
	}
}

#include "meshes/brush.cpp"

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

enum EditTool {
	Select,
};


glm::vec3 object_new_pos(glm::vec3 cur_pos, glm::vec3 plane, glm::vec3 origin, glm::vec3 dir, u32 grid) {
	float griddiv = 256.0 / pow(2.0f, (float)grid);
	griddiv = 1.0 / griddiv;

	glm::vec3 res = glm::length((cur_pos - origin) * plane) / glm::length(dir * plane) * dir;
	glm::vec3 jonathan = glm::vec3(round((origin + res) / griddiv) * griddiv) * (glm::vec3(1.0f) - plane);
	// printf("jort\n");
	// printf("%s\n", glm::to_string(jonathan + cur_pos * plane).c_str());
	// printf("%s\n", glm::to_string(plane).c_str());
	// printf("%s\n", glm::to_string(cur_pos * plane).c_str());

	return jonathan + cur_pos * plane;
}

glm::vec3 get_gizmo_plane_(char* name) {
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

glm::vec3 get_gizmo_plane(char* name) {
	return glm::vec3(1) - get_gizmo_plane_(name);
}

struct Gizmo {
	glm::vec3 pos;
	Model *model;
	int visible;
};

int main () { // impure
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER | SDL_INIT_SENSOR);

	SDL_Window *window = SDL_CreateWindow("fortnite moment", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1920, 1080, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

	SDL_JoystickEventState(SDL_ENABLE);

	SDL_GameController *gc = SDL_GameControllerOpen(0);
	SDL_GameControllerSetSensorEnabled(gc, SDL_SENSOR_GYRO, SDL_TRUE);

	if (SDL_GameControllerHasSensor(gc, SDL_SENSOR_GYRO)) {
		printf("fortnite\n");
	}


	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GLContext gl_context = SDL_GL_CreateContext(window);
	gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress);


	glViewport(0, 0, 1920, 1080);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init();

	cgltf_options options{};
	cgltf_data *data = NULL;
	cgltf_result result = cgltf_parse_file(&options, "res/models/tgizmo.glb", &data);
	cgltf_load_buffers(&options, data, "./");

	Gizmo gizmo;
	gizmo.pos = glm::vec3(0.0f);
	gizmo.model = gltf_upload_model(data);
	gizmo.visible = 1;

	cgltf_free(data);


	result = cgltf_parse_file(&options, "ak47.glb", &data);
	cgltf_load_buffers(&options, data, "./");

	Model *box = gltf_upload_model(data);

	cgltf_free(data);


	u32 vert_shader = loadShader("fortnite.vert", GL_VERTEX_SHADER);
	u32 frag_shader = loadShader("fortnite.frag", GL_FRAGMENT_SHADER);
	u32 grid_fshader = loadShader("grid.frag", GL_FRAGMENT_SHADER);

    u32 textv = loadShader("text.vert", GL_VERTEX_SHADER);
    u32 textf = loadShader("text.frag", GL_FRAGMENT_SHADER);

	u32 shader = createProgram(vert_shader, frag_shader, false);
	u32 grid_shader = createProgram(vert_shader, grid_fshader, true);
    u32 text_shader = createProgram(textv, textf, true);

	int quit = 0;
	SDL_Event event;

	int width = 1920, height = 1080;

	u32 time_old = 0;

	glm::quat rots_stored;

	Camera *camera = (Camera*)malloc(sizeof(Camera));
	camera->origin = glm::vec3(3.0f, 3.0f, 3.0f);
	camera->fov = 90.0f;
	camera->pitch = 0.5f;
	camera->yaw = glm::radians(-45.0f);
	// camera->rotation = glm::normalize(glm::quat());

	int rel_mode = 0;


	//SDL_GL_SetSwapInterval(0);

	u32 frametime_total = 0;
	float frametime = 0.0;
	float fps = 0.0;

	int frames = 0;


	u64 last_delta_uint = 0;


	int x, y, channels;
	stbi_uc *texture_data = stbi_load("textures/cobble.png", &x, &y, &channels, 0);

	u32 tex;
	glGenTextures(1, &tex);

	glBindTexture(GL_TEXTURE_2D, tex);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, 16.0f);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture_data);
	glGenerateMipmap(GL_TEXTURE_2D);

	stbi_image_free(texture_data);

    ObjectList objects = new_object_list();

    Object ak;
    ak.model = box;
    ak.classname = "ak47";

    objects = add_object(objects, &ak);

    Object ak2;
    ak2.model = box;
    ak2.classname = "ak473";

    objects = add_object(objects, &ak2);

	BrushList brushes = new_brushlist();

	MaterialList materials = new_materiallist();
	Brush *brush = new_brush();
	brushes = add_brush(brushes, brush);

	float vertices[] = {
		50.0, 0.0, -50.0, 1.0, 0.0,
		-50.0, 0.0, -50.0, 0.0, 0.0,
		-50.0, 0.0,	50.0, 0.0, 1.0,

		-50.0, 0.0,	50.0, 0.0, 1.0,
		50.0, 0.0,	50.0, 1.0, 1.0,
		50.0, 0.0, -50.0, 1.0, 0.0,
	};

	u32 grid_vbo;
	glGenBuffers(1, &grid_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, grid_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	i32 size;
	glGetNamedFramebufferAttachmentParameteriv(0, GL_DEPTH, GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE, &size);
	printf("%u\n", size);

	u32 vao;
	glGenVertexArrays(1, &vao);

	float lines[] = {0.0, -100.0, 0.0, 0.0, 100.0, 0.0};
	u32 line_vbo;
	glGenBuffers(1, &line_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, line_vbo);
	glBufferData(GL_ARRAY_BUFFER, 2 * 3 * sizeof(float), lines, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	u32 line_num = 0;

	i32 grid = 7;

	float mouse_depth = 1.0;

	int mx, my;
	SDL_GetMouseState(&mx, &my);

	u32 moving = 0;
	u32 vertical = 0;

	EditMode edit_mode = ObjectM;

	u32 wireframe = 0;

	glm::vec3 init_jort;
	std::vector<glm::vec3> init_jorts;
	glm::vec3 init_fort;

	std::vector<Selection> selected_brushes;

	glm::vec3 move_dir;
	int gizmoind = 0;

	UploadedMesh *aabb_mesh;
	AABB *aabb_brushes = 0;
	// glm::vec3 gizmo_center = glm::vec3(0.0f);

	u32 extruded = 0;

    char *selected_object_class = "fortnite balls";

	while (!quit) {
		u32 cur_time = SDL_GetTicks();
		// printf("%u - %u, %u\n", cur_time, time_old, delta_8);
		u32 delta_int = cur_time - time_old;
		frametime_total += delta_int;
		if (frametime_total > 0.1) {
			frametime = (float)frametime_total / 1000.0 / (float)frames;
			frametime_total = 0;
			fps = 1.0 / frametime;
			frames = 0;
		}
		u64 delta_uint = SDL_GetPerformanceCounter() - last_delta_uint;
		last_delta_uint = SDL_GetPerformanceCounter();
		double delta_ = (double)delta_uint	/ (double)SDL_GetPerformanceFrequency();
		//printf("%lf, %lf, %lu\n", fps, 1.0 / delta_, SDL_GetPerformanceFrequency());
		float delta = (float)(delta_int) / 1000.0f;
		time_old = cur_time;

		camera->rotation = cam::rotation(camera);

		glm::mat4 view = glm::mat4(1.0f);
		view = glm::toMat4(camera->rotation) * glm::translate(view, -camera->origin);
		glm::mat4 proj = glm::perspective(glm::radians(camera->fov), (float)width/(float)height, 0.1f, 100.0f);

		float sens = 0.004;

		glm::vec3 ray_world;
		{
			float ndx = (2.0 * mx) / width - 1.0;
			float ndy = 1.0 - (2.0 * my) / height;
			glm::vec4 clip = glm::vec4(ndx, ndy, -1.0, 1.0);

			glm::vec4 ray_eye = glm::inverse(proj) * clip;
			ray_eye = glm::vec4(ray_eye.x, ray_eye.y, -1.0, 0.0);

			glm::vec3 ray_world_ = glm::vec3(glm::inverse(view) * ray_eye);
			ray_world = glm::normalize(ray_world_);
		}

		glm::vec3 world_0 = glm::vec3(view * glm::vec4(0.0, 0.0, 0.0, 1.0));

		// glm::vec3 jortnite = glm::unProject(glm::vec3(mx, my, -1.0), view, proj, glm::vec4(0.0, 0.0, (float)width, (float)height));
		// printf("%f\n", mouse_depth);
		// printf("%s\n", glm::to_string(glm::normalize(jortnite)).c_str());
		// printf("%s\n", glm::to_string(world_0).c_str());

		// handle SDL events
		while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);

			switch (event.type) {
				case SDL_QUIT:
					quit = 1;
					break;
				case SDL_WINDOWEVENT:
					switch (event.window.event) {
						case SDL_WINDOWEVENT_RESIZED:
							glViewport(0, 0, event.window.data1, event.window.data2);
							width = event.window.data1;
							height = event.window.data2;
					}
					break;
				case SDL_MOUSEMOTION:
					// camera->rotation = glm::normalize(glm::quat(glm::vec3(event.motion.yrel * sens, event.motion.xrel * sens, 0.0f)) + camera->rotation);
					if (rel_mode) {
						camera->yaw += event.motion.xrel * sens;
						camera->pitch += event.motion.yrel * sens;
						if (camera->pitch > 1.5f) {
							camera->pitch = 1.5f;
						} else if (camera->pitch < -1.5f) {
							camera->pitch = -1.5f;
						}
						// printf("%f, %f\n", camera->yaw, camera->pitch);
					}
					mx = event.motion.x;
					my = event.motion.y;
					break;
				case SDL_MOUSEBUTTONDOWN:
					switch (event.button.button) {
						case SDL_BUTTON_RIGHT:
							rel_mode = true;
							// SDL_SetRelativeMouseMode(SDL_TRUE);
							break;
						case SDL_BUTTON_LEFT:
							// glm::vec3 temp = glm::vec3(glm::vec4(0.0f, 0.0f, -1.0f, 1.0f) * camera->rotation);
							// temp = glm::normalize(temp);
							{

								float dist;
								u32 jorter = select_brush(brushes, camera->origin, ray_world, &dist);

								int is_present = 0;
								if (moving == 2 && gizmo.visible) {
									for (int i = 0; i < gizmo.model->count; i++) {
										float diste;
										GltfMesh *meshe = &gizmo.model->meshes[i];
										AABB temp;
										temp.max = gizmo.model->transform * meshe->transform * glm::vec4(meshe->aabb->max, 1.0);
										temp.min = gizmo.model->transform * meshe->transform * glm::vec4(meshe->aabb->min, 1.0);
										if (intersect_ray_aabb(&temp, camera->origin, ray_world, &diste)) {
											is_present = 1;
										}
									}
								}
								u32 idx = 0;

									switch (edit_mode) {
										case Vertex: {
											//Brush *brush = get_brush(brushes, selected_brushes[0].obj);
											int selected_vert = -1;
											int selected_brush = -1;
											float shortest_dist = INFINITY;
											for (int i = 0; i < brushes.len; i++) {
												Brush *brush = get_brush(brushes, i);
												intersect_brush(brush, camera->origin, ray_world, &dist);
												for (int j = 0; j < brush->mesh->verts.size(); j++) {
													glm::vec3 vert = brush->mesh->verts[j];
													glm::vec2 screen_pos = glm::vec2(glm::project(vert, view * get_brush_matrix(brush), proj, glm::vec4(0, 0, width, -height)));
													screen_pos.y += height;
													float cur_dist = glm::length2(glm::vec2(mx, my) - screen_pos);
													if (mx > screen_pos.x - 50 && my > screen_pos.y - 50 &&
															mx < screen_pos.x + 50 && my < screen_pos.y + 50 &&
															cur_dist < shortest_dist &&
															dist + 0.1 >= glm::length(camera->origin - vert)) {
														shortest_dist = cur_dist;
														selected_vert = j;
														selected_brush = i;
													}
												}
											}
											if (selected_vert == -1) {
												if (~SDL_GetModState() & KMOD_LSHIFT && !is_present) {
													selected_brushes.clear();
													init_jorts.clear();
													if (jorter == -1) {
														moving = 5;
													}
												}
												break;
											}

											int brush_elem = -1;
											int vert_elem = 0;
											for (int i = 0; i < selected_brushes.size(); i++) {
												if (selected_brushes[i].obj == selected_brush) {
													brush_elem = i;
													for (int j = 0; j < selected_brushes[i].verts.size(); j++) {
														if (selected_brushes[i].verts[j] == selected_vert) {
															vert_elem = 1;
															break;
														}
													}
													break;
												}
											}
											if (selected_brush != -1 && !is_present) {
												if (brush_elem == -1) {
													selected_brushes.push_back(Selection { (u32)selected_brush });
													brush_elem = selected_brushes.size() - 1;
												}
												if (brush_elem != -1 && !vert_elem) {
													selected_brushes[brush_elem].verts.push_back(selected_vert);
												}
												for (u32 i : selected_brushes[brush_elem].verts) {
													printf("%i\n", i);
												}
											}
										}
										break;
										case Face:
										case ObjectM: {
											if (~SDL_GetModState() & KMOD_LSHIFT && !is_present) {
												selected_brushes.clear();
												init_jorts.clear();
												if (jorter == -1) {
													moving = 5;
												}
											}

											int elem = 0;
											for (int i = 0; i < selected_brushes.size(); i++) {
												if (selected_brushes[i].obj == jorter) {
													elem = 1;
													idx = i;
												}
											}
											if (!elem) {
												if (jorter != -1 && !is_present) {
													idx = selected_brushes.size();
													selected_brushes.push_back(Selection { jorter });
												}
											}

											if (jorter != -1) {
												init_jort = get_brush(brushes, selected_brushes[0].obj)->origin;
											}

											if (selected_brushes.size()) {
												Brush *new_brush = get_brush(brushes, selected_brushes[0].obj);
												gizmo.pos = new_brush->origin;
											}

											if (edit_mode == ObjectM) break;

											if (selected_brushes.size()) {
												int selected_face = -1;
												for (int i = 0; i < selected_brushes.size(); i++) {
													Brush *jortware = get_brush(brushes, selected_brushes[i].obj);
													selected_face = select_brush_face(jortware, camera->origin, ray_world);
													if (selected_face != -1) {
														break;
													}
												}
												int face_elem = 0;
												for (u32 i : selected_brushes[idx].faces) {
													if (selected_face == i) {
														face_elem = 1;
													}
												}
												if (~SDL_GetModState() & KMOD_LSHIFT && !face_elem && !is_present) {
													selected_brushes[idx].faces.clear();
												}
												if (!face_elem && !is_present) {
													selected_brushes[idx].faces.push_back(selected_face);
													Brush *new_brush = get_brush(brushes, selected_brushes[idx].obj);
													gizmo.pos = get_face_center(new_brush->mesh, selected_brushes[idx].faces[0]) + new_brush->origin;
													//printf("%s\n", glm::to_string(gizmo.pos).c_str());
												}
												for (u32 i : selected_brushes[idx].faces) {
													printf("%i\n", i);
												}
											}
											break;
										}
									}
								if (!moving && selected_brushes.size()) {
									moving = 1;
								}
								if (selected_brushes.size() && is_present) {
									if (moving && moving != 5) {
										moving = 3;
									}
								}
							}
							break;
					}
					break;
				case SDL_MOUSEBUTTONUP:
					switch (event.button.button) {
						case SDL_BUTTON_RIGHT:
							rel_mode = false;
							// SDL_SetRelativeMouseMode(SDL_FALSE);
							break;
						case SDL_BUTTON_LEFT:
							if (moving == 5) {
								moving = 0;
							} else if (moving) {
								moving = 2;
							}
							extruded = 0;
							break;
					}
					break;
				case SDL_MOUSEWHEEL:
					{
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
						// move_face_along_normal(brushes.data[selected_brush].mesh, selected_face, dir, grid);
						// brushes.data[selected_brush] = *update_brush(&brushes.data[selected_brush]);
						// break;
					}
				case SDL_KEYDOWN:
					switch (event.key.keysym.scancode) {
						case SDL_SCANCODE_E:
							grid -= 1;
							break;
						case SDL_SCANCODE_Q:
							// if (grid != 0) {
							grid += 1;
							// }
							break;
						case SDL_SCANCODE_B:
							brushes = add_brush(brushes, new_brush());
							break;
						case SDL_SCANCODE_G:
							break;
						case SDL_SCANCODE_3:
							edit_mode = Face;
							break;
						case SDL_SCANCODE_1:
							edit_mode = Vertex;
							break;
						case SDL_SCANCODE_5:
							edit_mode = ObjectM;
							break;
						case SDL_SCANCODE_EQUALS:
							wireframe = !wireframe;
							break;
						case SDL_SCANCODE_C:
							for (Selection i : selected_brushes) {
								brushes = remove_brush(brushes, i.obj);
							}
							selected_brushes.clear();
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
			cam::translate_z(camera, -speed * delta);
		}
		if (kb[SDL_SCANCODE_W]) {
			cam::translate_z(camera, speed * delta);
		}
		if (kb[SDL_SCANCODE_D]) {
			cam::translate_x(camera, -speed * delta);
		}
		if (kb[SDL_SCANCODE_A]) {
			cam::translate_x(camera, speed * delta);
		}
		if (kb[SDL_SCANCODE_SPACE]) {
			cam::translate_y(camera, speed * delta);
		}
		if (kb[SDL_SCANCODE_LALT]) {
			vertical = 1;
		} else {
			vertical = 0;
		}


        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        ImGui::Text(selected_object_class);

        ImVec2 button_sz(100, 100);
        ImGuiStyle& style = ImGui::GetStyle();
        float window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
        for (int i = 0; i < objects.len; i++) {
            if (ImGui::Button(objects.data[i]->classname, button_sz)) {
                selected_object_class = objects.data[i]->classname;
            }
            float last_button_x2 = ImGui::GetItemRectMax().x;
            float next_button_x2 = last_button_x2 + style.ItemSpacing.x + button_sz.x; // Expected position if next button was on same line
            if (i + 1 < objects.len && next_button_x2 < window_visible_x2)
                ImGui::SameLine();
        }

		// float rots[3];
		// printf("%f, %f, %f\n", rots_stored[0], rots_stored[1], rots_stored[2]);
		// float gyro_sens = 1.0f;
		// SDL_GameControllerGetSensorData(gc, SDL_SENSOR_GYRO, rots, 3);
		// glm::vec3 rots_vec = glm::vec3(rots[0], rots[1], rots[2]);
		// glm::quat rot_quat = glm::quat(rots_vec * delta * gyro_sens);
		// rots_stored = glm::normalize(rots_stored * rot_quat);
		// glm::mat4 model = glm::mat4(1.0f) * glm::toMat4(rots_stored);

		// printf("%f, %f, %f, %f\n", camera->rotation.w, camera->rotation.x, camera->rotation.y, camera->rotation.z);
		// printf("%f, %f, %f\n", camera->origin.x, camera->origin.y, camera->origin.z);


		float diste;
		float best_dist = INFINITY;
		if (moving == 2 && gizmo.visible) {
			gizmoind = -1;
			for (int i = 0; i < gizmo.model->count; i++) {
				GltfMesh *meshe = &gizmo.model->meshes[i];
				AABB temp;
				temp.max = gizmo.model->transform * meshe->transform * glm::vec4(meshe->aabb->max, 1.0);
				temp.min = gizmo.model->transform * meshe->transform * glm::vec4(meshe->aabb->min, 1.0);
				if (intersect_ray_aabb(&temp, camera->origin, ray_world, &diste) && diste < best_dist) {
					gizmoind = i;
					best_dist = diste;
				}
			}
			if (gizmoind != -1) {
				move_dir = get_gizmo_plane(gizmo.model->meshes[gizmoind].name);
			} else {
				move_dir = glm::vec3(1);
			}
		}

		glClearColor(0.0, 0.5, 1.0, 1.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glm::mat4 model = glm::mat4(1.0f);

		glUseProgram(grid_shader);
		glEnable(GL_BLEND);
		glDisable(GL_CULL_FACE);
		glEnable(GL_DEPTH_TEST);

		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		// // glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(model));
		// // cubeb.origin = glm::vec3(glm::vec3(round(camera->origin.x), 0.0, round(camera->origin.z)));
		glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(model));
		glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(proj));
		glUniform3fv(5, 1, glm::value_ptr(camera->origin));

		glBindVertexArray(vao);

		glBindBuffer(GL_ARRAY_BUFFER, grid_vbo);

		glUniform1f(4, pow(2.0f, (float)grid));

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 12 + 8, 0);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 12 + 8, (void*)12);
		glEnableVertexAttribArray(2);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glUseProgram(shader);
		glEnable(GL_CULL_FACE);
		glDisable(GL_BLEND);

		model = glm::mat4(1.0f);
		glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(model));
		glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(proj));

		glUniform1ui(9, 0);
		//glUniform1ui(9, -1);
		glUniform1ui(5, 0); // has_tex = 0

		glm::vec3 albedo = glm::vec3(0.0, 1.0, 0.0);

		glUniform3fv(4, 1, glm::value_ptr(albedo));
		glBindBuffer(GL_ARRAY_BUFFER, line_vbo);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 12, 0);
		glEnableVertexAttribArray(0);
		glDrawArrays(GL_LINES, 0, 2);

		albedo = glm::vec3(1.0);
		glUniform3fv(4, 1, glm::value_ptr(albedo));

		if (moving == 3 && gizmo.visible) {
			if (selected_brushes.size()) {
				// printf("jort\n");
				switch (edit_mode) {
					case ObjectM: {
						Brush *brusher = get_brush(brushes, selected_brushes[0].obj);
						brusher->origin = object_new_pos(brusher->origin, move_dir, camera->origin, ray_world, grid);
						gizmo.pos = brusher->origin;

						glm::vec3 orig_mod = brusher->origin - init_jort;
						for (int i = 0; i < selected_brushes.size(); i++) {
							if (i == 0) {
								continue;
							}
							brush = get_brush(brushes, selected_brushes[i].obj);
							brush->origin += orig_mod;
						}
						init_jort = brusher->origin;
						break;
					}
					case Face: {
						glm::vec3 offs = gizmo.pos;
						gizmo.pos = object_new_pos(gizmo.pos, move_dir, camera->origin, ray_world, grid);
						offs = gizmo.pos - offs;
						//if (offs != glm::vec3(0.0, 0.0, 0.0)) {
						//printf("%s\n", glm::to_string(offs).c_str());
						//}
						for (int i = 0; i < selected_brushes.size(); i++) {
							Brush *cur_brush = get_brush(brushes, selected_brushes[i].obj);
							for (int j = 0; j < selected_brushes[i].faces.size(); j++) {
								u32 cur_face = selected_brushes[i].faces[j];
								if (offs != glm::vec3(0.0, 0.0, 0.0)) {
									if (SDL_GetModState() & KMOD_LSHIFT && !extruded) {
										extrude_face(cur_brush->mesh, cur_face);
									}
								}
							}
							move_faces(cur_brush->mesh, selected_brushes[i].faces, offs);
							update_brush(cur_brush);
						}
						extruded = 1;
						break;
					}
					case Vertex: {
						glm::vec3 offs = gizmo.pos;
						gizmo.pos = object_new_pos(gizmo.pos, move_dir, camera->origin, ray_world, grid);
						offs = gizmo.pos - offs;
						for (int i = 0; i < selected_brushes.size(); i++) {
							Brush *cur_brush = get_brush(brushes, selected_brushes[i].obj);
							for (int j = 0; j < selected_brushes[i].verts.size(); j++) {
								cur_brush->mesh->verts[selected_brushes[i].verts[j]] += offs;
							}
							update_brush(cur_brush);
						}
					}
				}
			}
		}
		glUniform1ui(10, 0);
		glUniform1ui(11, 1);

		glEnable(GL_BLEND);
		if (edit_mode == Vertex) {
			render_brushes_points(brushes, selected_brushes);
		}
		if (wireframe) {
			glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
			glDisable(GL_CULL_FACE);
		} else {
			glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
			glEnable(GL_CULL_FACE);
		}
        glBindTexture(GL_TEXTURE_2D, tex);
		render_brushes(brushes, selected_brushes);

		glUniform1ui(11, 0);
		glUniform1ui(10, 0);


		glUniform1ui(9, 0);
		if (moving > 1 && moving < 4 && gizmo.visible) {
			glm::vec3 gizmo_dir = glm::normalize(gizmo.pos - camera->origin);
			gizmo.model->transform = glm::translate(glm::mat4(1.0f), camera->origin + gizmo_dir * glm::vec3(15.0f));
			glClear(GL_DEPTH_BUFFER_BIT);
			if (gizmoind != -1) {
				render_model_jort(gizmo.model, gizmoind);
			} else {
				render_model(gizmo.model);
			}
		}
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		SDL_GL_SwapWindow(window);
		frames++;
	}



    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
	SDL_DestroyWindow(window);
	SDL_Quit();
}
