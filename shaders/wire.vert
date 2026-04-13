#version 410 core
layout(location = 0) in vec3 aPos;
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform float uCurrentY;
void main() {
    vec3 pos = aPos;
    pos.y += uCurrentY + 0.005; // slight bias to avoid z-fighting
    gl_Position = uProjection * uView * uModel * vec4(pos, 1.0);
}