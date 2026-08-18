#version 450
layout(location = 0) in vec3 fragColor;
layout(location = 1) in flat int fragInstance;
layout(location = 0) out vec4 outColor;

void main() {
    float brightness = 0.6 + 0.4 * (float(fragInstance) / 8.0);
    outColor = vec4(fragColor * brightness, 1.0);
}
