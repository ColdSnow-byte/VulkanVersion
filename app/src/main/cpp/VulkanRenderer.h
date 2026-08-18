#ifndef VULKAN_RENDERER_H
#define VULKAN_RENDERER_H

#include <vulkan/vulkan.h>
#include <android/native_window.h>
#include <cstdint>
#include <string>
#include <vector>

// 与 Java 层约定的版本代号
constexpr int VK_API_1_0 = 0;
constexpr int VK_API_1_1 = 1;
constexpr int VK_API_1_3 = 2;
constexpr int VK_API_1_4 = 3;

// 渲染器基类：封装 Vulkan 通用初始化（instance/device/swapchain）
// 各版本子类负责具体渲染管线与特性使用。
class VulkanRenderer {
public:
    VulkanRenderer(ANativeWindow* window, int version);
    virtual ~VulkanRenderer();

    bool init(std::string* errorMsg);
    void drawFrame();
    void onSurfaceChanged(int width, int height);
    void updateUniforms();   // 每帧更新 UBO（旋转动画）

    int getVersion() const { return version_; }

protected:
    // ---- 通用资源 ----
    VkInstance instance_ = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;
    VkQueue graphicsQueue_ = VK_NULL_HANDLE;
    uint32_t graphicsQueueFamily_ = 0;
    VkSurfaceKHR surface_ = VK_NULL_HANDLE;
    VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
    std::vector<VkImage> swapchainImages_;
    std::vector<VkImageView> swapchainImageViews_;
    std::vector<VkFramebuffer> framebuffers_;   // 仅 1.1 使用（传统 renderpass）
    VkFormat swapchainImageFormat_ = VK_FORMAT_UNDEFINED;
    VkExtent2D swapchainExtent_{};
    VkCommandPool commandPool_ = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers_;
    VkSemaphore imageAvailableSemaphore_ = VK_NULL_HANDLE;
    VkSemaphore renderFinishedSemaphore_ = VK_NULL_HANDLE;
    VkFence inFlightFence_ = VK_NULL_HANDLE;

    // ---- 深度缓冲（1.3/1.4 需要深度） ----
    VkImage depthImage_ = VK_NULL_HANDLE;
    VkDeviceMemory depthImageMemory_ = VK_NULL_HANDLE;
    VkImageView depthImageView_ = VK_NULL_HANDLE;

    // ---- 渲染循环状态 ----
    ANativeWindow* window_ = nullptr;
    int version_;
    int width_ = 0;
    int height_ = 0;
    bool framebufferResized_ = false;
    bool initialized_ = false;
    uint64_t frameCount_ = 0;

    // ---- 通用初始化步骤 ----
    bool createInstance(std::string* errorMsg);
    bool pickPhysicalDevice(std::string* errorMsg);
    bool createLogicalDevice(std::string* errorMsg);
    bool createSwapchain();
    void createDepthResources();
    void createCommandPool();
    void createSyncObjects();
    void cleanupSwapchain();
    void recreateSwapchain();

    // ---- 通用辅助 ----
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags props);
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                      VkMemoryPropertyFlags props, VkBuffer& buffer,
                      VkDeviceMemory& memory);
    void createImage(uint32_t w, uint32_t h, VkFormat format, VkImageTiling tiling,
                     VkImageUsageFlags usage, VkMemoryPropertyFlags props,
                     VkImage& image, VkDeviceMemory& memory);
    VkImageView createImageView(VkImage image, VkFormat format,
                                VkImageAspectFlags aspectFlags);
    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer cmd);
    void copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size);
    void transitionImageLayout(VkImage image, VkFormat format,
                               VkImageLayout oldLayout, VkImageLayout newLayout);

    // ---- 子类需要实现的接口 ----
    virtual bool initGraphicsPipeline(std::string* errorMsg) = 0;
    virtual void cleanupGraphicsPipeline() = 0;
    virtual void recordCommandBuffer(VkCommandBuffer cmd, uint32_t imageIndex) = 0;
    virtual void createRenderPassResources() = 0;   // 子类按需创建 framebuffer / 动态渲染视图
    virtual void writeUniformData() = 0;            // 子类写入 UBO 数据（旋转动画）

    // ---- 子类可用的成员 ----
    VkRenderPass renderPass_ = VK_NULL_HANDLE;      // 1.1 使用
    VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;
    VkPipeline graphicsPipeline_ = VK_NULL_HANDLE;
};

#endif // VULKAN_RENDERER_H
