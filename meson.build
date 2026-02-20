project(
  'mapedit',
  ['c', 'cpp'],
  version : '0.1',
  meson_version : '>= 1.3.0',
  default_options : ['warning_level=3', 'cpp_std=c++14'],
)

dependencies = [
  dependency('sdl2')
]

exe = executable(
  'mapedit',

  'src/main.cpp',
  'src/camera.cpp',
  'src/meshes/editing.cpp',
  'src/physics/aabb.cpp',
  'src/renderer/gltf.cpp',
  'src/meshes/brush.cpp',
  'src/renderer/shader.cpp',
  'src/scene.cpp',
  '3rdparty/flecs/flecs.c',

	# gcc -lstdc++ -o vulkan -lSDL2 -lEGL -lGL -lOpenGL -lm src/main.cpp -I3rdparty -ggdb -Werror=return-type -I3rdparty/imgui -I3rdparty/imgui/backends 3rdparty/imgui/backends/imgui_impl_sdl2.o 3rdparty/imgui/backends/imgui_impl_opengl3.o 3rdparty/imgui/imgui*.o 3rdparty/flecs/flecs.o -fpermissive
  objects: [
    '3rdparty/imgui/backends/imgui_impl_sdl2.o',
    '3rdparty/imgui/backends/imgui_impl_opengl3.o',
    '3rdparty/imgui/imgui.o',
    '3rdparty/imgui/imgui_demo.o',
    '3rdparty/imgui/imgui_draw.o',
    '3rdparty/imgui/imgui_tables.o',
    '3rdparty/imgui/imgui_widgets.o'
  ],

  cpp_args: ['-Wno-switch'],
  dependencies : dependencies,
  include_directories: include_directories(['3rdparty', '3rdparty/imgui', '3rdparty/imgui/backends', 'src']))

test('basic', exe)
