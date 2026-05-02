#version 410 core

layout(location = 0) in vec2 aPos;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform float uWaterLevel;
uniform float uTime;

out float vFlowOffset;
out float vPulseIntensity;

void main() {
    // Calculate flow offset for fragment shader
    float posSum = aPos.x * 0.3 + aPos.y * 0.7;
    vFlowOffset = sin(posSum * 2.0 + uTime * 1.5) * cos(posSum * 1.5) * 0.5 + 0.5;
    
    // Pulse intensity for width/thickness variation
    vPulseIntensity = 0.7 + 0.3 * sin(posSum * 3.0 + uTime * 2.0);
    
    // Slight vertical movement for wave effect
    float waveHeight = sin(posSum * 4.0 + uTime * 2.0) * 0.03;
    
    // Place river with slight wave animation
    vec3 worldPos = vec3(aPos.x, uWaterLevel + 0.05 + waveHeight, aPos.y);
    
    gl_Position = uProjection * uView * uModel * vec4(worldPos, 1.0);
}