#version 410 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aUV;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform float uTime;

out vec3 vWorldPos;
out vec2 vUV;
out vec3 vNormal;

// Gerstner wave function
vec3 gerstnerWave(vec3 pos, vec2 dir, float steepness, float wavelength) {
    float k = 2.0 * 3.14159 / wavelength;
    float c = sqrt(9.8 / k);
    float f = k * (dot(dir, pos.xz) - c * uTime);
    float a = steepness / k;
    return vec3(
        dir.x * a * cos(f),
        a * sin(f),
        dir.y * a * cos(f)
    );
}

void main() {
    vec3 pos = aPos;

    // Combine multiple wave frequencies for realistic ocean
    pos += gerstnerWave(pos, normalize(vec2(1.0, 0.8)), 0.06, 4.2);
    pos += gerstnerWave(pos, normalize(vec2(-0.6, 1.0)), 0.04, 2.8);
    pos += gerstnerWave(pos, normalize(vec2(0.3, -1.0)), 0.025, 1.6);

    vWorldPos = vec3(uModel * vec4(pos, 1.0));
    vUV       = aUV;

    // Approximate normal from wave derivatives (flat normal + wave tilt)
    vNormal = normalize(vec3(-0.08 * sin(uTime + pos.x), 1.0, -0.05 * sin(uTime * 0.7 + pos.z)));

    gl_Position = uProjection * uView * vec4(vWorldPos, 1.0);
}
