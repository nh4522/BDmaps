#version 410 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

uniform float uCurrentY;   // spring-animated vertical offset
uniform float uTime;

out vec3 vWorldPos;
out vec3 vNormal;
out vec2 vUV;
out float vHeight; // 0 = base, 1 = top

void main() {
    // Translate entire district up by animated offset
    vec3 pos = aPos;
    pos.y += uCurrentY;

    vHeight   = pos.y;
    vWorldPos = vec3(uModel * vec4(pos, 1.0));
    vNormal   = normalize(mat3(transpose(inverse(uModel))) * aNormal);
    vUV       = aUV;

    gl_Position = uProjection * uView * vec4(vWorldPos, 1.0);
}
