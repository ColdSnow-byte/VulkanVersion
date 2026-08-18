#version 450

// Vulkan 1.0 最原始用法：顶点数据硬编码在着色器中，
// 不使用 UBO / 描述符集 / 矩阵变换，仅通过 Push Constants 传递一个时间偏移。

layout(push_constant) uniform Push {
    float time;
} pc;

layout(location = 0) out vec3 fragColor;

void main() {
    // 硬编码的四边形（两个三角形共 6 个顶点），通过 gl_VertexIndex 生成
    vec2 positions[6] = vec2[6](
        vec2(-0.7, -0.7),
        vec2( 0.7, -0.7),
        vec2(-0.7,  0.7),

        vec2(-0.7,  0.7),
        vec2( 0.7, -0.7),
        vec2( 0.7,  0.7)
    );

    // 根据顶点位置给不同区域上色
    vec3 color = vec3(
        positions[gl_VertexIndex].x * 0.5 + 0.5,
        positions[gl_VertexIndex].y * 0.5 + 0.5,
        abs(sin(pc.time)) * 0.5 + 0.5
    );

    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    fragColor = color;
}
