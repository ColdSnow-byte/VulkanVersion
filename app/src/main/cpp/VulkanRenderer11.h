#ifndef VULKAN_RENDERER_11_H
#define VULKAN_RENDERER_11_H

#include "VulkanRenderer.h"

// Vulkan 1.1 渲染器：
// 使用基础图形管线 + 传统 RenderPass/Framebuffer，
// 渲染一个旋转的彩色三角形（顶点颜色插值）。
class VulkanRenderer11 : public VulkanRenderer {
public:
    VulkanRenderer11(ANativeWindow* window) : VulkanRenderer(window, VK_API_1_1) {}
    ~VulkanRenderer11() override { cleanupGraphicsPipeline(); }

protected:
    bool initGraphicsPipeline(std::string* errorMsg) override;
    void cleanupGraphicsPipeline() override;
    void createRenderPassResources() override;
    void recordCommandBuffer(VkCommandBuffer cmd, uint32_t imageIndex) override;
    void writeUniformData() override;

private:
    VkBuffer vertexBuffer_ = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory_ = VK_NULL_HANDLE;
    VkBuffer uniformBuffer_ = VK_NULL_HANDLE;
    VkDeviceMemory uniformBufferMemory_ = VK_NULL_HANDLE;
    void* uniformBufferMapped_ = nullptr;

    VkDescriptorSetLayout descriptorSetLayout_ = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool_ = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSet_ = VK_NULL_HANDLE;
};

#endif // VULKAN_RENDERER_11_H
