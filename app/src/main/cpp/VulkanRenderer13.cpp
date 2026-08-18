#include "VulkanRenderer13.h"
#include "CommonData.h"
#include "shaders/vert_common.h"
#include "shaders/frag_common.h"
#include <cstring>

// 立方体：每个面的 4 个顶点颜色不同，片元插值后产生渐变（类似 1.1 三角形的效果）
static const std::vector<Vertex> CUBE_VERTICES = {
    // 前（红→黄→绿→蓝 渐变）
    {{-0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}},   // 左下 红
    {{ 0.5f, -0.5f,  0.5f}, {1.0f, 1.0f, 0.0f}},   // 右下 黄
    {{ 0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}},   // 右上 绿
    {{-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}},   // 左上 蓝
    // 后（绿→青→蓝→品红 渐变）
    {{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},   // 右下 绿
    {{-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 1.0f}},   // 左下 青
    {{-0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}},   // 左上 蓝
    {{ 0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 1.0f}},   // 右上 品红
    // 右（蓝→青→绿→黄 渐变）
    {{ 0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}},   // 前下 蓝
    {{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 1.0f}},   // 后下 青
    {{ 0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},   // 后上 绿
    {{ 0.5f,  0.5f,  0.5f}, {1.0f, 1.0f, 0.0f}},   // 前上 黄
    // 左（黄→品红→红→蓝 渐变）
    {{-0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}},   // 后下 黄
    {{-0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 1.0f}},   // 前下 品红
    {{-0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}},   // 前上 红
    {{-0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}},   // 后上 蓝
    // 上（品红→黄→白→青 渐变）
    {{-0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 1.0f}},   // 前左 品红
    {{ 0.5f,  0.5f,  0.5f}, {1.0f, 1.0f, 0.0f}},   // 前右 黄
    {{ 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}},   // 后右 白
    {{-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 1.0f}},   // 后左 青
    // 下（青→蓝→品红→红 渐变）
    {{-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 1.0f}},   // 后左 青
    {{ 0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}},   // 后右 蓝
    {{ 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 1.0f}},   // 前右 品红
    {{-0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}},   // 前左 红
};

static const std::vector<uint16_t> CUBE_INDICES = {
    0, 1, 2,  2, 3, 0,   // 前
    4, 5, 6,  6, 7, 4,   // 后
    8, 9, 10, 10, 11, 8, // 右
    12, 13, 14, 14, 15, 12, // 左
    16, 17, 18, 18, 19, 16, // 上
    20, 21, 22, 22, 23, 20, // 下
};

static VkShaderModule createShaderModule(VkDevice device, const uint32_t* code, size_t size) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = size;
    createInfo.pCode = code;
    VkShaderModule shaderModule;
    vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule);
    return shaderModule;
}

bool VulkanRenderer13::initGraphicsPipeline(std::string* errorMsg) {
    // ---- 顶点缓冲 ----
    VkDeviceSize vSize = sizeof(CUBE_VERTICES[0]) * CUBE_VERTICES.size();
    createBuffer(vSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 vertexBuffer_, vertexBufferMemory_);
    void* data;
    vkMapMemory(device_, vertexBufferMemory_, 0, vSize, 0, &data);
    memcpy(data, CUBE_VERTICES.data(), (size_t)vSize);
    vkUnmapMemory(device_, vertexBufferMemory_);

    // ---- 索引缓冲 ----
    VkDeviceSize iSize = sizeof(CUBE_INDICES[0]) * CUBE_INDICES.size();
    createBuffer(iSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 indexBuffer_, indexBufferMemory_);
    vkMapMemory(device_, indexBufferMemory_, 0, iSize, 0, &data);
    memcpy(data, CUBE_INDICES.data(), (size_t)iSize);
    vkUnmapMemory(device_, indexBufferMemory_);

    // ---- Uniform ----
    createBuffer(sizeof(UBO), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 uniformBuffer_, uniformBufferMemory_);
    vkMapMemory(device_, uniformBufferMemory_, 0, sizeof(UBO), 0, &uniformBufferMapped_);

    // ---- 描述符 ----
    VkDescriptorSetLayoutBinding layoutBinding{};
    layoutBinding.binding = 0;
    layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layoutBinding.descriptorCount = 1;
    layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &layoutBinding;
    vkCreateDescriptorSetLayout(device_, &layoutInfo, nullptr, &descriptorSetLayout_);

    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = 1;
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = 1;
    vkCreateDescriptorPool(device_, &poolInfo, nullptr, &descriptorPool_);

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool_;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &descriptorSetLayout_;
    vkAllocateDescriptorSets(device_, &allocInfo, &descriptorSet_);

    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = uniformBuffer_;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UBO);
    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = descriptorSet_;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pBufferInfo = &bufferInfo;
    vkUpdateDescriptorSets(device_, 1, &descriptorWrite, 0, nullptr);

    // ---- 管线布局 ----
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout_;
    vkCreatePipelineLayout(device_, &pipelineLayoutInfo, nullptr, &pipelineLayout_);

    // ---- 着色器 ----
    VkShaderModule vertModule = createShaderModule(device_, vert_common_spv, sizeof(vert_common_spv));
    VkShaderModule fragModule = createShaderModule(device_, frag_common_spv, sizeof(frag_common_spv));

    VkPipelineShaderStageCreateInfo vertStage{};
    vertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertStage.module = vertModule;
    vertStage.pName = "main";

    VkPipelineShaderStageCreateInfo fragStage{};
    fragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragStage.module = fragModule;
    fragStage.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertStage, fragStage };

    auto bindingDesc = Vertex::getBindingDescription();
    auto attrDescs = Vertex::getAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDesc;
    vertexInputInfo.vertexAttributeDescriptionCount = (uint32_t)attrDescs.size();
    vertexInputInfo.pVertexAttributeDescriptions = attrDescs.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;   // 不裁剪，依靠深度测试保证立方体正确显示
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // 深度模板状态
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = (uint32_t)dynamicStates.size();
    dynamicState.pDynamicStates = dynamicStates.data();

    // 动态渲染：使用 1.3 核心动态渲染，无 renderPass
    VkPipelineRenderingCreateInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachmentFormats = &swapchainImageFormat_;
    renderingInfo.depthAttachmentFormat = VK_FORMAT_D16_UNORM;

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = &renderingInfo;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = pipelineLayout_;
    pipelineInfo.renderPass = VK_NULL_HANDLE; // 动态渲染无需 renderpass
    pipelineInfo.subpass = 0;

    if (vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr,
                                  &graphicsPipeline_) != VK_SUCCESS) {
        if (errorMsg) *errorMsg = "创建 Vulkan 1.3 图形管线失败";
        return false;
    }

    vkDestroyShaderModule(device_, vertModule, nullptr);
    vkDestroyShaderModule(device_, fragModule, nullptr);
    return true;
}

void VulkanRenderer13::createRenderPassResources() {
    // 动态渲染不需要 RenderPass 和 Framebuffer
    // 深度视图已在基类 createDepthResources 中创建
}

void VulkanRenderer13::recordCommandBuffer(VkCommandBuffer cmd, uint32_t imageIndex) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(cmd, &beginInfo);

    // ---- 使用 synchronization2 的 image memory barrier 转换颜色图像布局 ----
    VkImageMemoryBarrier2 colorBarrier{};
    colorBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    colorBarrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    colorBarrier.srcAccessMask = 0;
    colorBarrier.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    colorBarrier.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    colorBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorBarrier.image = swapchainImages_[imageIndex];
    colorBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    colorBarrier.subresourceRange.baseMipLevel = 0;
    colorBarrier.subresourceRange.levelCount = 1;
    colorBarrier.subresourceRange.baseArrayLayer = 0;
    colorBarrier.subresourceRange.layerCount = 1;

    VkImageMemoryBarrier2 depthBarrier{};
    depthBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    depthBarrier.srcStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT;
    depthBarrier.srcAccessMask = 0;
    depthBarrier.dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT;
    depthBarrier.dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    depthBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthBarrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depthBarrier.image = depthImage_;
    depthBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    depthBarrier.subresourceRange.baseMipLevel = 0;
    depthBarrier.subresourceRange.levelCount = 1;
    depthBarrier.subresourceRange.baseArrayLayer = 0;
    depthBarrier.subresourceRange.layerCount = 1;

    VkImageMemoryBarrier2 barriers[] = { colorBarrier, depthBarrier };

    VkDependencyInfo dependencyInfo{};
    dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dependencyInfo.imageMemoryBarrierCount = 2;
    dependencyInfo.pImageMemoryBarriers = barriers;

    vkCmdPipelineBarrier2(cmd, &dependencyInfo);

    // ---- 动态渲染开始 ----
    VkRenderingAttachmentInfo colorAttachment{};
    colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachment.imageView = swapchainImageViews_[imageIndex];
    colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.clearValue.color = {{0.15f, 0.12f, 0.2f, 1.0f}};

    VkRenderingAttachmentInfo depthAttachment{};
    depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    depthAttachment.imageView = depthImageView_;
    depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.clearValue.depthStencil = { 1.0f, 0 };

    VkRenderingInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea.offset = {0, 0};
    renderingInfo.renderArea.extent = swapchainExtent_;
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorAttachment;
    renderingInfo.pDepthAttachment = &depthAttachment;

    vkCmdBeginRendering(cmd, &renderingInfo);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline_);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)swapchainExtent_.width;
    viewport.height = (float)swapchainExtent_.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapchainExtent_;
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    VkBuffer vertexBuffers[] = { vertexBuffer_ };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(cmd, indexBuffer_, 0, VK_INDEX_TYPE_UINT16);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout_,
                            0, 1, &descriptorSet_, 0, nullptr);
    vkCmdDrawIndexed(cmd, (uint32_t)CUBE_INDICES.size(), 1, 0, 0, 0);

    vkCmdEndRendering(cmd);

    // ---- 转换颜色图像回 PRESENT 布局（synchronization2） ----
    VkImageMemoryBarrier2 presentBarrier{};
    presentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    presentBarrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    presentBarrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    presentBarrier.dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
    presentBarrier.dstAccessMask = 0;
    presentBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    presentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    presentBarrier.image = swapchainImages_[imageIndex];
    presentBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    presentBarrier.subresourceRange.baseMipLevel = 0;
    presentBarrier.subresourceRange.levelCount = 1;
    presentBarrier.subresourceRange.baseArrayLayer = 0;
    presentBarrier.subresourceRange.layerCount = 1;

    VkDependencyInfo presentDependency{};
    presentDependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    presentDependency.imageMemoryBarrierCount = 1;
    presentDependency.pImageMemoryBarriers = &presentBarrier;

    vkCmdPipelineBarrier2(cmd, &presentDependency);

    vkEndCommandBuffer(cmd);
}

void VulkanRenderer13::writeUniformData() {
    if (uniformBufferMapped_ == nullptr) return;

    UBO ubo{};
    float angle = frameCount_ * 0.02f;
    ubo.model = Mat4::rotationX(angle) * Mat4::rotationY(angle * 0.7f);
    ubo.view = Mat4::translation(0.0f, 0.0f, -3.0f);
    float aspect = swapchainExtent_.width / (float)swapchainExtent_.height;
    ubo.proj = Mat4::perspective(radians(45.0f), aspect, 0.1f, 10.0f);

    memcpy(uniformBufferMapped_, &ubo, sizeof(ubo));
}

void VulkanRenderer13::cleanupGraphicsPipeline() {
    if (device_ == VK_NULL_HANDLE) return;
    if (uniformBufferMapped_) {
        vkUnmapMemory(device_, uniformBufferMemory_);
        uniformBufferMapped_ = nullptr;
    }
    if (uniformBuffer_ != VK_NULL_HANDLE) vkDestroyBuffer(device_, uniformBuffer_, nullptr);
    if (uniformBufferMemory_ != VK_NULL_HANDLE) vkFreeMemory(device_, uniformBufferMemory_, nullptr);
    if (vertexBuffer_ != VK_NULL_HANDLE) vkDestroyBuffer(device_, vertexBuffer_, nullptr);
    if (vertexBufferMemory_ != VK_NULL_HANDLE) vkFreeMemory(device_, vertexBufferMemory_, nullptr);
    if (indexBuffer_ != VK_NULL_HANDLE) vkDestroyBuffer(device_, indexBuffer_, nullptr);
    if (indexBufferMemory_ != VK_NULL_HANDLE) vkFreeMemory(device_, indexBufferMemory_, nullptr);
    if (descriptorPool_ != VK_NULL_HANDLE) vkDestroyDescriptorPool(device_, descriptorPool_, nullptr);
    if (descriptorSetLayout_ != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(device_, descriptorSetLayout_, nullptr);
}
