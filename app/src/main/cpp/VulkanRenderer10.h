#ifndef VULKAN_RENDERER_10_H
#define VULKAN_RENDERER_10_H

#include "VulkanRenderer.h"

// Vulkan 1.0 渲染器：
// 使用最原始的 1.0 能力 —— Push Constants（无 UBO、无描述符集、无矩阵变换），
// 顶点数据硬编码在着色器中，仅通过 vkCmdPushConstants 传递一个时间参数，
// 渲染一个静态的渐变色四边形。
// 与 1.1（UBO + 描述符集 + 矩阵变换的旋转三角形）形成鲜明对比。
class VulkanRenderer10 : public VulkanRenderer {
public:
    VulkanRenderer10(ANativeWindow* window) : VulkanRenderer(window, VK_API_1_0) {}
    ~VulkanRenderer10() override { cleanupGraphicsPipeline(); }

protected:
    bool initGraphicsPipeline(std::string* errorMsg) override;
    void cleanupGraphicsPipeline() override;
    void createRenderPassResources() override;
    void recordCommandBuffer(VkCommandBuffer cmd, uint32_t imageIndex) override;
    void writeUniformData() override;

private:
    // 1.0 不使用 UBO/描述符集/顶点缓冲，仅使用 Push Constants（顶点由 gl_VertexIndex 生成）
};

#endif // VULKAN_RENDERER_10_H
