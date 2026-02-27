#version 460 core

layout (location = 0) in vec2 texPos;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 fragPos;
layout (location = 3) in flat uint facei;

layout (location = 0) out vec4 fragColor;

layout (location = 4) uniform vec3 albedo;

void main () {
    fragColor = vec4(albedo, 1.0);
}
