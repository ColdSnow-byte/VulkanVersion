#ifndef VULKAN_RENDERER_14_H
#define VULKAN_RENDERER_14_H

#include "VulkanRenderer.h"

// Vulkan 1.4 渲染器：
// 使用 VK_KHR_push_descriptor（推送描述符，无需描述符池/描述符集）
// 和 VK_KHR_dynamic_rendering + VK_KHR_synchronization2，
// 并利用实例化渲染（instancing）绘制一个旋转的星形阵列。
class VulkanRenderer14 : public VulkanRenderer {
public:
    VulkanRenderer14(ANativeWindow* window) : VulkanRenderer(window, VK_API_1_4) {}
    ~VulkanRenderer14() override { cleanupGraphicsPipeline(); }

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

    // 动态加载的推送描述符函数（NDK libvulkan.so 未导出 1.4 核心函数）
    PFN_vkCmdPushDescriptorSetKHR vkCmdPushDescriptorSetKHR_ = nullptr;

    static constexpr int INSTANCE_COUNT = 8;
};

#endif // VULKAN_RENDERER_14_H
