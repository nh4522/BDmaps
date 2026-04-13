#version 410 core

in vec3 vWorldPos;
in vec2 vUV;
in vec3 vNormal;

uniform vec3  uCameraPos;
uniform vec3  uLightDir;
uniform float uTime;

out vec4 FragColor;

void main() {
    vec3 viewDir = normalize(uCameraPos - vWorldPos);
    vec3 normal  = normalize(vNormal);

    // Deep vs shallow water colour
    vec3 deepColor    = vec3(0.02, 0.08, 0.22);
    vec3 shallowColor = vec3(0.06, 0.22, 0.38);
    float depth = clamp(vWorldPos.y * 2.0 + 0.5, 0.0, 1.0);
    vec3 waterColor = mix(deepColor, shallowColor, depth);

    // Diffuse
    float diff = max(dot(normal, uLightDir), 0.0);
    waterColor += diff * vec3(0.05, 0.12, 0.20);

    // Specular highlight (sun glitter)
    vec3 halfDir = normalize(uLightDir + viewDir);
    float spec   = pow(max(dot(normal, halfDir), 0.0), 128.0);
    vec3 specular = spec * vec3(1.0, 0.97, 0.9) * 0.7;

    // Fresnel — more reflective at grazing angles
    float fresnel = pow(1.0 - max(dot(normal, viewDir), 0.0), 4.0);
    vec3 reflectColor = vec3(0.5, 0.65, 0.85) * fresnel * 0.4;

    // Animated foam pattern
    float foam = smoothstep(0.65, 0.85,
        sin(vUV.x * 12.0 + uTime * 0.5) * sin(vUV.y * 9.0 + uTime * 0.4)
    );
    waterColor = mix(waterColor, vec3(0.85, 0.9, 1.0), foam * 0.15);

    vec3 finalColor = waterColor + specular + reflectColor;
    float alpha     = 0.82 + fresnel * 0.12;

    FragColor = vec4(finalColor, alpha);
}
