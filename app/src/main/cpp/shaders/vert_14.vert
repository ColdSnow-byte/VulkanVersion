#version 450
layout(binding = 0) uniform UBO {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out flat int fragInstance;

void main() {
    float angle = float(gl_InstanceIndex) * (6.2831853 / 8.0);
    float radius = 1.2;
    vec3 offset = vec3(cos(angle) * radius, sin(angle) * radius, 0.0);

    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition * 0.4 + offset, 1.0);
    fragColor = inColor;
    fragInstance = gl_InstanceIndex;
}
