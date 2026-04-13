#version 410 core

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vUV;
in float vHeight;

uniform vec3  uBaseColor;
uniform vec3  uLightColor;
uniform vec3  uLightDir;
uniform vec3  uCameraPos;
uniform float uHovered;     // 0.0 or 1.0
uniform float uSelected;    // 0.0 or 1.0
uniform float uTime;
uniform int   uIsSide;      // 1 for side faces

out vec4 FragColor;

// Simple Blinn-Phong + rim lighting
vec3 lighting(vec3 color, vec3 normal, vec3 viewDir) {
    // Ambient
    vec3 ambient = color * 0.28;

    // Diffuse
    float diff = max(dot(normal, uLightDir), 0.0);
    vec3 diffuse = diff * color * uLightColor;

    // Specular (Blinn-Phong)
    vec3 halfDir  = normalize(uLightDir + viewDir);
    float spec    = pow(max(dot(normal, halfDir), 0.0), 32.0);
    vec3 specular = spec * uLightColor * 0.4;

    // Rim light (edge glow)
    float rim = 1.0 - max(dot(viewDir, normal), 0.0);
    rim = pow(rim, 3.0);
    vec3 rimColor = uBaseColor * rim * 0.3;

    return ambient + diffuse + specular + rimColor;
}

void main() {
    vec3 normal  = normalize(vNormal);
    vec3 viewDir = normalize(uCameraPos - vWorldPos);

    vec3 color = uBaseColor;

    // Side faces are slightly darker
    if (uIsSide == 1) {
        color *= 0.65;
    }

    // Hover/select: brighten and tint
    if (uHovered > 0.5) {
        color = mix(color, color * 1.6 + vec3(0.08, 0.12, 0.18), 0.55);
    }
    if (uSelected > 0.5) {
        // Pulsing selection glow
        float pulse = 0.5 + 0.5 * sin(uTime * 3.0);
        color = mix(color * 1.4, vec3(0.9, 0.95, 1.0), pulse * 0.25);
    }

    vec3 lit = lighting(color, normal, viewDir);

    // Subtle edge darkening based on UV (shows polygon boundary)
    float edgeDist = min(vUV.x, min(1.0 - vUV.x, min(vUV.y, 1.0 - vUV.y)));
    lit *= (0.92 + 0.08 * smoothstep(0.0, 0.05, edgeDist));

    // Height-based colour shift (tops slightly brighter)
    lit += vec3(0.0, 0.02, 0.04) * vHeight;

    FragColor = vec4(lit, 1.0);
}
