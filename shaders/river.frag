#version 410 core

in float vFlowOffset;
in float vPulseIntensity;

uniform vec3 uColorDeep;
uniform vec3 uColorShallow;
uniform float uTime;

out vec4 FragColor;

void main() {
    // Flowing animation
    float flow = sin(vFlowOffset * 10.0 + uTime * 3.0) * 0.4 + 0.6;
    
    // Color variation based on flow and pulse
    vec3 color = mix(uColorDeep, uColorShallow, vFlowOffset * 0.6 + flow * 0.2);
    
    // Add animated sparkles
    float sparkle = pow(max(0.0, sin(vFlowOffset * 30.0 + uTime * 5.0)), 6.0);
    color += vec3(0.4, 0.6, 0.9) * sparkle * vPulseIntensity;
    
    // Edge brightness pulsing
    color *= (0.8 + 0.3 * vPulseIntensity);
    
    FragColor = vec4(color, 0.85);
}