#version 460 core

layout (location = 0) in vec2 texPos;

layout (location = 0) out vec4 fragColor;

layout (location = 4) uniform float grid;
layout (location = 5) uniform vec3 cam_pos;

void main() {
    vec2 tex_n = texPos - 0.5;
    float balls = 256.0 / grid;
    vec2 jort = tex_n * 100.0 * balls;
    vec2 tex_m = mod(tex_n * 100.0 * balls, 1.0);
    float alpha = 1.0;
    // float dist = gl_FragCoord.z / gl_FragCoord.w / 2.0;
    vec3 base_color = vec3(tex_m, 1.0);//vec3(0.0);
    base_color = vec3(0.0);
    vec2 fac = 1.0 - abs(texPos * 2.0 - 1.0);
    float epsilon = 0.1 / 2.0;
    alpha = fac.x * fac.y;

    if (jort.x < epsilon && jort.x > -epsilon) {
        base_color = vec3(1.0, 0.0, 0.0);
        fragColor = vec4(base_color, alpha);
    }

    if (jort.y < epsilon && jort.y > -epsilon) {
        base_color = vec3(0.0, 0.0, 1.0);
        fragColor = vec4(base_color, alpha);
    }

    if (tex_m.x > epsilon && tex_m.x < (1.0 - epsilon) && tex_m.y > epsilon && tex_m.y < (1.0 - epsilon)) {
        discard;
    }
    fragColor = vec4(base_color, alpha);
}
