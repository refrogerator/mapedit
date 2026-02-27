#version 460 core

layout (location = 0) in vec2 texPos;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 fragPos;
layout (location = 3) in flat uint facei;

layout (location = 0) out vec4 fragColor;

layout (location = 4) uniform vec3 albedo;
layout (location = 5) uniform bool has_tex;

struct Material {
    int fortnite;
};

layout (location = 6) uniform bool selected;
layout (location = 8) uniform vec3 viewPos;

layout (location = 7) uniform sampler2D tex;
layout (location = 9) uniform uint jortnite_len;

layout (location = 10) uniform bool gridt;
layout (location = 11) uniform bool indexed;

#define NR_SELECTED_FACES 100
layout (location = 12) uniform uint[NR_SELECTED_FACES] jortnite;

uniform vec4 overlay;

vec3 hsv2rgb(vec3 c) {
  vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
  vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
  return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec4 grid(vec2 pos, vec4 base_color) {
    vec2 pos_ = mod(pos * 2, 1.0);
    // vec2 pos__ = pos + 0.5;
    vec2 pos__ = texPos;

    float epsilon = 0.1 / 2.0;

    if (pos_.x > epsilon && pos_.x < (1.0 - epsilon) && pos_.y > epsilon && pos_.y < (1.0 - epsilon)) {
        return base_color;
    }

    return vec4(vec3(0.0), 1.0);
    // return vec4(vec3(pos.x / 2.0 + pos.y / 2.0), 1.0);
    // return vec4(vec3(pos.x+0.5, pos.y+0.5, 1.0), 1.0);
}

vec4 triplanar_map(vec4 base_color) {
    float jort = 0;
    int fart = 0;

    if (abs(dot(normalize(abs(normal)), vec3(1.0, 0.0, 0.0))) > jort) {
        fart = 0;
    }
    if (abs(dot(normalize(abs(normal)), vec3(0.0, 1.0, 0.0))) > jort) {
        fart = 1;
    }
    if (abs(dot(normalize(abs(normal)), vec3(0.0, 0.0, 1.0))) > jort) {
        fart = 2;
    }

    switch (fart) {
        case 0: return grid(fragPos.yz, base_color);
            break;
        case 1: return grid(fragPos.xz, base_color);
            break;
        case 2: return grid(fragPos.xy, base_color);
            break;
    }
}

void main () {
    vec3 lightColor = vec3(1.0);
    vec3 lightPos = vec3(1.0, 1.0, 1.0);

    vec4 baseColor;

    vec3 norm = normalize(normal);
    vec3 lightDir = normalize(lightPos - fragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    if (has_tex) {
        baseColor = texture(tex, texPos) * vec4(albedo, 1.0);
    } else {
        baseColor = vec4(albedo, 1.0);
    }

    //fragColor = vec4(diffuse, 1.0) * baseColor;
    //return;

	bool elem = false;
	for (int i = 0; i < jortnite_len; i++) {
		if (facei == jortnite[i]) {
			elem = true;
			break;
		}
	}

    if (indexed && elem) {
        // baseColor = vec4(1.0);
        float overlayAlpha = 0.5;
        fragColor = (baseColor * (1.0 - overlayAlpha)) + vec4(vec3(0.0, 0.8, 0.8) * overlayAlpha, 1.0);
        //fragColor = vec4(vec3(fragColor) * triplanar_map(), fragColor.a);
		// fragColor = vec4(1.0);
        return;
    }
    //fragColor = vec4(normal + 1.0 / 2.0, 1.0);
	fragColor = baseColor;
    if (gridt) {
        fragColor = triplanar_map(baseColor);
    }
    fragColor = (fragColor * (1.0 - overlay.a)) + vec4(overlay.rgb * overlay.a, 1.0);
    // if (gl_PrimitiveID)
    // fragColor = vec4(hsv2rgb(vec3(float(gl_PrimitiveID) / 24.0, 1.0, 1.0)), 1.0);
    return;

    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * lightColor;



    float specularStrength = 0.5;

    vec3 viewDir = normalize(viewPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, norm);

    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;

    vec3 result = (ambient + diffuse + specular);

    // fragColor = vec4(result, 1.0) * baseColor;
}
