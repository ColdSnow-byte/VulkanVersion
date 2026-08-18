#ifndef VULKAN_RENDERER_13_H
#define VULKAN_RENDERER_13_H

#include "VulkanRenderer.h"

// Vulkan 1.3 渲染器：
// 使用 VK_KHR_dynamic_rendering（动态渲染，无需 RenderPass/Framebuffer）
// 和 VK_KHR_synchronization2（同步2）特性，
// 渲染一个旋转的彩色立方体（含深度测试）。
class VulkanRenderer13 : public VulkanRenderer {
public:
    VulkanRenderer13(ANativeWindow* window) : VulkanRenderer(window, VK_API_1_3) {}
    ~VulkanRenderer13() override { cleanupGraphicsPipeline(); }

protected:
    bool initGraphicsPipeline(std::string* errorMsg) override;
    void cleanupGraphicsPipeline() override;
    void createRenderPassResources() override;
    void recordCommandBuffer(VkCommandBuffer cmd, uint32_t imageIndex) override;
    void writeUniformData() override;

private:
    VkBuffer vertexBuffer_ = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory_ = VK_NULL_HANDLE;
    VkBuffer indexBuffer_ = VK_NULL_HANDLE;
    VkDeviceMemory indexBufferMemory_ = VK_NULL_HANDLE;
    VkBuffer uniformBuffer_ = VK_NULL_HANDLE;
    VkDeviceMemory uniformBufferMemory_ = VK_NULL_HANDLE;
    void* uniformBufferMapped_ = nullptr;

    VkDescriptorSetLayout descriptorSetLayout_ = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool_ = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSet_ = VK_NULL_HANDLE;
};

#endif // VULKAN_RENDERER_13_H
