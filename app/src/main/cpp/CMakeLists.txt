cmake_minimum_required(VERSION 3.22.1)
project(vulkanversion)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 使用 Android 平台 Vulkan Surface 扩展
add_definitions(-DVK_USE_PLATFORM_ANDROID_KHR)

add_library(vulkanversion SHARED
        native.cpp
        VulkanRenderer.cpp
        VulkanRenderer10.cpp
        VulkanRenderer11.cpp
        VulkanRenderer13.cpp
        VulkanRenderer14.cpp
)

target_include_directories(vulkanversion PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/vulkan_headers
)

# 使用本地最新 Vulkan 头文件（支持 1.4），优先于 NDK 自带旧头文件
target_include_directories(vulkanversion BEFORE PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/vulkan_headers
)

find_library(android-lib android)
find_library(log-lib log)

target_link_libraries(vulkanversion
        vulkan
        ${android-lib}
        ${log-lib}
)
