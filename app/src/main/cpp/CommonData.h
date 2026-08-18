#ifndef COMMON_DATA_H
#define COMMON_DATA_H

#include <vulkan/vulkan.h>
#include <vector>
#include <cmath>
#include <cstring>

// 简单的 4x4 矩阵（列主序，与 GLSL mat4 对齐）
struct Mat4 {
    float m[16];

    Mat4() { setIdentity(); }

    void setIdentity() {
        memset(m, 0, sizeof(m));
        m[0] = m[5] = m[10] = m[15] = 1.0f;
    }

    static Mat4 perspective(float fovy, float aspect, float znear, float zfar) {
        Mat4 r;
        memset(r.m, 0, sizeof(r.m));
        float f = 1.0f / tanf(fovy / 2.0f);
        r.m[0] = f / aspect;
        r.m[5] = f;
        r.m[10] = zfar / (znear - zfar);
        r.m[11] = -1.0f;
        r.m[14] = (zfar * znear) / (znear - zfar);
        return r;
    }

    static Mat4 rotationZ(float angle) {
        Mat4 r;
        float c = cosf(angle);
        float s = sinf(angle);
        r.m[0] = c;  r.m[1] = s;
        r.m[4] = -s; r.m[5] = c;
        return r;
    }

    static Mat4 rotationY(float angle) {
        Mat4 r;
        float c = cosf(angle);
        float s = sinf(angle);
        r.m[0] = c;  r.m[2] = -s;
        r.m[8] = s;  r.m[10] = c;
        return r;
    }

    static Mat4 rotationX(float angle) {
        Mat4 r;
        float c = cosf(angle);
        float s = sinf(angle);
        r.m[5] = c;  r.m[6] = s;
        r.m[9] = -s; r.m[10] = c;
        return r;
    }

    static Mat4 translation(float x, float y, float z) {
        Mat4 r;
        r.m[12] = x; r.m[13] = y; r.m[14] = z;
        return r;
    }

    Mat4 operator*(const Mat4& o) const {
        Mat4 r;
        memset(r.m, 0, sizeof(r.m));
        for (int c = 0; c < 4; c++) {
            for (int row = 0; row < 4; row++) {
                float sum = 0.0f;
                for (int k = 0; k < 4; k++) {
                    sum += m[k * 4 + row] * o.m[c * 4 + k];
                }
                r.m[c * 4 + row] = sum;
            }
        }
        return r;
    }
};

// 顶点结构（position + color）
struct Vertex {
    float pos[3];
    float color[3];

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription binding{};
        binding.binding = 0;
        binding.stride = sizeof(Vertex);
        binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return binding;
    }

    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
        std::vector<VkVertexInputAttributeDescription> attrs(2);
        attrs[0].binding = 0;
        attrs[0].location = 0;
        attrs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attrs[0].offset = offsetof(Vertex, pos);
        attrs[1].binding = 0;
        attrs[1].location = 1;
        attrs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attrs[1].offset = offsetof(Vertex, color);
        return attrs;
    }
};

// 每帧更新的 Uniform（MVP 矩阵）
struct UBO {
    Mat4 model;
    Mat4 view;
    Mat4 proj;
};

// 角度转弧度
inline float radians(float degrees) {
    return degrees * 0.0174532925f;
}

#endif // COMMON_DATA_H
