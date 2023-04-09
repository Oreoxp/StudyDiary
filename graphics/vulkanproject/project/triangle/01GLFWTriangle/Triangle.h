#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <optional>
#include <set>
#include <stdexcept>
#include <vector>
#include <optional>
#include <vulkan\vulkan.h>


//定义了队列族的索引，包含了图形队列和呈现队列
struct QueueFamilyIndices {
  std::optional<uint32_t> graphicsFamily;
  std::optional<uint32_t> presentFamily;

  //检查是否有图形队列和呈现队列
  bool isComplete() {
    return graphicsFamily.has_value() && presentFamily.has_value();
  }
};

//交换链支持的详细信息
struct SwapChainSupportDetails {
  //交换链的能力
  VkSurfaceCapabilitiesKHR capabilities;
  //交换链支持的像素格式
  std::vector<VkSurfaceFormatKHR> formats;
  //交换链支持的呈现模式
  std::vector<VkPresentModeKHR> presentModes;
};

class HelloTriangleApplication {
 public:
  void run();


private:

  //初始化窗口
  void initWindow();

  //初始化Vulkan
  void initVulkan();

  void mainLoop();

  void cleanup();
  
  void cleanupSwapChain();

  void createInstance();

  void populateDebugMessengerCreateInfo(
      VkDebugUtilsMessengerCreateInfoEXT& createInfo);

  void setupDebugMessenger();

  void createSurface();

  void pickPhysicalDevice();

  void createLogicalDevice();

  void createSwapChain();

  void createImageViews();

  void createRenderPass();

  void createGraphicsPipeline();

  void createFramebuffers();

  void createCommandPool();

  void createCommandBuffer();

  void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

  void createSyncObjects();

  void drawFrame();

  VkShaderModule createShaderModule(const std::vector<char>& code);

  VkSurfaceFormatKHR chooseSwapSurfaceFormat(
      const std::vector<VkSurfaceFormatKHR>& availableFormats);

  VkPresentModeKHR chooseSwapPresentMode(
      const std::vector<VkPresentModeKHR>& availablePresentModes);

  VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

  SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

  bool isDeviceSuitable(VkPhysicalDevice device);

  bool checkDeviceExtensionSupport(VkPhysicalDevice device);

  QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

  std::vector<const char*> getRequiredExtensions();

  bool checkValidationLayerSupport();

  static std::vector<char> readFile(const std::string& filename);

  static VKAPI_ATTR VkBool32 VKAPI_CALL
  debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                VkDebugUtilsMessageTypeFlagsEXT messageType,
                const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                void* pUserData);

  void reCreateSwapChain();

  static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

 private:
  GLFWwindow* window;

  //Vulkan实例
  VkInstance instance;
  //调试信息
  VkDebugUtilsMessengerEXT debugMessenger;
  //呈现表面
  VkSurfaceKHR surface;

  //物理设备
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
  VkDevice device;

  //图形队列和呈现队列
  VkQueue graphicsQueue;
  VkQueue presentQueue;

  //交换链
  VkSwapchainKHR swapChain;
  //交换链中的图像
  std::vector<VkImage> swapChainImages;
  //交换链中的图像格式
  VkFormat swapChainImageFormat;
  //交换链中的图像大小
  VkExtent2D swapChainExtent;
  //交换链中的图像视图
  std::vector<VkImageView> swapChainImageViews;
  //交换链中的帧缓冲
  std::vector<VkFramebuffer> swapChainFramebuffers;

  //渲染通道
  VkRenderPass renderPass;
  //图形管线布局
  VkPipelineLayout pipelineLayout;
  //图形管线
  VkPipeline graphicsPipeline;

  //命令池
  VkCommandPool commandPool;
  //命令缓冲
  VkCommandBuffer commandBuffer;

  //信号量
  VkSemaphore imageAvailableSemaphore;
  VkSemaphore renderFinishedSemaphore;
  //栅栏
  VkFence inFlightFence;

  bool framebufferResized = false;
};
