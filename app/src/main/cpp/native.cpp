#include <jni.h>
#include <android/native_window_jni.h>
#include <android/log.h>
#include <vulkan/vulkan.h>
#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <cmath>
#include <vector>
#include <memory>

#include "VulkanRenderer.h"
#include "VulkanRenderer10.h"
#include "VulkanRenderer11.h"
#include "VulkanRenderer13.h"
#include "VulkanRenderer14.h"
#include "CommonData.h"

#define LOG_TAG "VulkanVersionNative"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// 全局状态
static std::unique_ptr<VulkanRenderer> g_renderer;
static std::thread g_renderThread;
static std::atomic<bool> g_running{false};
static std::mutex g_mutex;

// JNI 环境缓存（用于回调）
static JavaVM* g_jvm = nullptr;
static jclass g_activityClass = nullptr;
static jobject g_activityObj = nullptr;

// 缓存的方法 ID
static jmethodID g_onMessageMethod = nullptr;
static jmethodID g_onUnsupportedMethod = nullptr;

// ==================== 回调辅助 ====================

static void cacheJniCallbacks(JNIEnv* env, jobject activity) {
    if (g_activityObj != nullptr) {
        env->DeleteGlobalRef(g_activityObj);
    }
    g_activityObj = env->NewGlobalRef(activity);

    jclass localClass = env->GetObjectClass(activity);
    g_activityClass = (jclass)env->NewGlobalRef(localClass);
    env->DeleteLocalRef(localClass);

    g_onMessageMethod = env->GetMethodID(g_activityClass, "onNativeMessage", "(Ljava/lang/String;)V");
    g_onUnsupportedMethod = env->GetMethodID(g_activityClass, "onNativeUnsupported", "(Ljava/lang/String;)V");
}

static void callOnMessage(const std::string& msg) {
    if (g_jvm == nullptr || g_activityObj == nullptr) return;
    JNIEnv* env = nullptr;
    bool attached = false;
    if (g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6) != JNI_OK) {
        if (g_jvm->AttachCurrentThread(&env, nullptr) == JNI_OK) {
            attached = true;
        }
    }
    if (env == nullptr) return;

    jstring jmsg = env->NewStringUTF(msg.c_str());
    env->CallVoidMethod(g_activityObj, g_onMessageMethod, jmsg);
    env->DeleteLocalRef(jmsg);

    if (attached) {
        g_jvm->DetachCurrentThread();
    }
}

static void callOnUnsupported(const std::string& msg) {
    if (g_jvm == nullptr || g_activityObj == nullptr) return;
    JNIEnv* env = nullptr;
    bool attached = false;
    if (g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6) != JNI_OK) {
        if (g_jvm->AttachCurrentThread(&env, nullptr) == JNI_OK) {
            attached = true;
        }
    }
    if (env == nullptr) return;

    jstring jmsg = env->NewStringUTF(msg.c_str());
    env->CallVoidMethod(g_activityObj, g_onUnsupportedMethod, jmsg);
    env->DeleteLocalRef(jmsg);

    if (attached) {
        g_jvm->DetachCurrentThread();
    }
}

// ==================== 版本检测 ====================

// 查询设备支持的最高 Vulkan 版本（读取物理设备的真实 apiVersion）
static int queryMaxSupportedVersion() {
    // 用最低版本 1.0 创建 instance（最宽松），然后读取物理设备实际版本
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Probe";
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    VkInstance inst = VK_NULL_HANDLE;
    if (vkCreateInstance(&createInfo, nullptr, &inst) != VK_SUCCESS) {
        return VK_API_1_1; // 兜底
    }

    uint32_t count = 0;
    vkEnumeratePhysicalDevices(inst, &count, nullptr);
    uint32_t maxApiVersion = VK_API_VERSION_1_0;
    if (count > 0) {
        std::vector<VkPhysicalDevice> devices(count);
        vkEnumeratePhysicalDevices(inst, &count, devices.data());
        // 取所有设备中的最高版本
        for (auto& d : devices) {
            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties(d, &props);
            if (props.apiVersion > maxApiVersion) {
                maxApiVersion = props.apiVersion;
            }
        }
    }
    vkDestroyInstance(inst, nullptr);

    // 映射到本应用支持的版本代号
    if (maxApiVersion >= VK_API_VERSION_1_4) return VK_API_1_4;
    if (maxApiVersion >= VK_API_VERSION_1_3) return VK_API_1_3;
    if (maxApiVersion >= VK_API_VERSION_1_1) return VK_API_1_1;
    // 1.0 设备
    return VK_API_1_0;
}

static std::string querySupportedVersionString() {
    // 枚举物理设备的 apiVersion
    std::string result = "未知";

    // 用 1.0 instance 探测（最宽松）
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Probe";
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    VkInstance inst = VK_NULL_HANDLE;
    if (vkCreateInstance(&createInfo, nullptr, &inst) != VK_SUCCESS) {
        return "设备不支持 Vulkan";
    }

    uint32_t count = 0;
    vkEnumeratePhysicalDevices(inst, &count, nullptr);
    if (count == 0) {
        vkDestroyInstance(inst, nullptr);
        return "未找到 Vulkan GPU";
    }

    std::vector<VkPhysicalDevice> devices(count);
    vkEnumeratePhysicalDevices(inst, &count, devices.data());

    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(devices[0], &props);

    uint32_t major = VK_VERSION_MAJOR(props.apiVersion);
    uint32_t minor = VK_VERSION_MINOR(props.apiVersion);
    uint32_t patch = VK_VERSION_PATCH(props.apiVersion);

    char buf[256];
    snprintf(buf, sizeof(buf), "%s (Vulkan %u.%u.%u)",
             props.deviceName, major, minor, patch);
    result = buf;

    vkDestroyInstance(inst, nullptr);
    return result;
}

// ==================== 渲染线程 ====================

static void renderLoop(ANativeWindow* window, int version) {
    LOGI("渲染线程启动，版本代号 %d", version);

    std::string errorMsg;
    VulkanRenderer* renderer = nullptr;

    switch (version) {
        case VK_API_1_0: renderer = new VulkanRenderer10(window); break;
        case VK_API_1_1: renderer = new VulkanRenderer11(window); break;
        case VK_API_1_3: renderer = new VulkanRenderer13(window); break;
        case VK_API_1_4: renderer = new VulkanRenderer14(window); break;
        default:
            callOnMessage("未知的 Vulkan 版本");
            ANativeWindow_release(window);
            return;
    }

    if (!renderer->init(&errorMsg)) {
        LOGE("渲染器初始化失败: %s", errorMsg.c_str());
        callOnUnsupported("渲染初始化失败：\n" + errorMsg);
        delete renderer;
        ANativeWindow_release(window);
        return;
    }

    {
        std::lock_guard<std::mutex> lock(g_mutex);
        g_renderer.reset(renderer);
    }

    callOnMessage("开始使用 Vulkan " +
                  std::string(version == VK_API_1_0 ? "1.0" :
                              version == VK_API_1_1 ? "1.1" :
                              version == VK_API_1_3 ? "1.3" : "1.4") + " 渲染");

    // 主渲染循环
    while (g_running.load()) {
        renderer->updateUniforms();
        renderer->drawFrame();

        // 简单休眠控制帧率（约 60fps）
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    // 析构函数会负责 vkDeviceWaitIdle 和资源清理
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        g_renderer.reset();
    }

    ANativeWindow_release(window);
    LOGI("渲染线程结束");
}

// ==================== JNI 导出 ====================

extern "C" JNIEXPORT jint JNICALL
JNI_OnLoad(JavaVM* vm, void* reserved) {
    g_jvm = vm;
    return JNI_VERSION_1_6;
}

extern "C" JNIEXPORT void JNICALL
Java_com_xxy_vulkanversion_MainActivity_nativeStartRender(
        JNIEnv* env, jobject thiz, jobject surface, jint version) {
    cacheJniCallbacks(env, thiz);

    // 停止已有渲染
    g_running.store(false);
    if (g_renderThread.joinable()) {
        g_renderThread.join();
    }

    ANativeWindow* window = ANativeWindow_fromSurface(env, surface);
    if (window == nullptr) {
        callOnMessage("无法获取 Surface");
        return;
    }

    g_running.store(true);
    g_renderThread = std::thread(renderLoop, window, version);
}

extern "C" JNIEXPORT void JNICALL
Java_com_xxy_vulkanversion_MainActivity_nativeStopRender(
        JNIEnv* env, jobject thiz) {
    g_running.store(false);
    if (g_renderThread.joinable()) {
        g_renderThread.join();
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_xxy_vulkanversion_MainActivity_nativeResize(
        JNIEnv* env, jobject thiz, jint width, jint height) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_renderer) {
        g_renderer->onSurfaceChanged(width, height);
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_xxy_vulkanversion_MainActivity_nativePause(
        JNIEnv* env, jobject thiz) {
    g_running.store(false);
    if (g_renderThread.joinable()) {
        g_renderThread.join();
    }
}

extern "C" JNIEXPORT jint JNICALL
Java_com_xxy_vulkanversion_MainActivity_nativeGetMaxSupportedVersion(
        JNIEnv* env, jobject thiz) {
    return queryMaxSupportedVersion();
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_xxy_vulkanversion_MainActivity_nativeGetSupportedVulkanVersion(
        JNIEnv* env, jobject thiz) {
    return env->NewStringUTF(querySupportedVersionString().c_str());
}
