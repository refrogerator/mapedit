#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexPos;
layout (location = 3) in uint facei_in;

layout (location = 0) out vec2 texPos;
layout (location = 1) out vec3 normal;
layout (location = 2) out vec3 fragPos;
layout (location = 3) out uint facei;

layout (location = 0) uniform mat4 model;
layout (location = 1) uniform mat4 view;
layout (location = 2) uniform mat4 projection;
layout (location = 6) uniform bool selected;

void main () {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    normal = mat3(transpose(inverse(model))) * aNormal;
    fragPos = vec3(model * vec4(aPos, 1.0));
    texPos = aTexPos;
	facei = facei_in;
}
