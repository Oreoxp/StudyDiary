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
#include <array>
#include <optional>
#include <vulkan\vulkan.h>
#include <glm/glm.hpp>


//定义了队列族的索引，包含了图形队列和呈现队列
struct QueueFamilyIndices {
  std::optional<uint32_t> graphicsFamily;
  std::optional<uint32_t> presentFamily;

  //检查是否有图形队列和呈现队列
  bool isComplete() {
    return graphicsFamily.has_value() && presentFamily.has_value();
  }
};

//MVP矩阵
struct UniformBufferObject {
  glm::vec2 foo;
  alignas(16) glm::mat4 model;
  glm::mat4 view;
  glm::mat4 proj;
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

struct Vertex {
  glm::vec2 pos;
  glm::vec3 color;

  static VkVertexInputBindingDescription getBindingDescription() {
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    //VK_VERTEX_INPUT_RATE_VERTEX: 每个顶点之后移动到下一个数据输入
    //VK_VERTEX_INPUT_RATE_INSTANCE: 每个实例之后移动到下一个数据输入
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return bindingDescription;
  }
  
  static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
    std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};
    //位置属性
    //binding: 告诉Vulkan每个顶点数据来自哪个绑定(binding)
    //location: 告诉Vulkan在着色器中的哪个位置(location)读取顶点数据
    //format: 告诉Vulkan如何解析顶点数据
    //    vec2：VK_FORMAT_R32G32_SINT，32位带符号整数的2组分量向量
    //    uvec4：VK_FORMAT_R32G32B32A32_UINT，32位无符号整数的4组分量向量
    //    double：VK_FORMAT_R64_SFLOAT，双精度（64位）浮点数
    //offset: 参数隐含定义了属性数据的字节大小，offset参数指定从每个顶点数据开始的字节数，要读取
    //的字节数。绑定一次加载一个Vertex，位置属性（pos）在此结构的开头偏移量为0字节。这是使用
    //offsetof宏自动计算的。
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, pos);
    //颜色属性
    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, color);


    return attributeDescriptions;
  }
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

  void createVertexBuffer();

  void createIndexBuffer();

  uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

  void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                    VkMemoryPropertyFlags properties, VkBuffer& buffer,
                    VkDeviceMemory& bufferMemory);

 void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) ;

 void createDescriptorSetLayout();

  void createUniformBuffers();

  void updateUniformBuffer(uint32_t currentImage);

  void createDescriptorPool();

  void createDescriptorSets();

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
  std::vector<VkCommandBuffer> commandBuffers;

  //信号量
  std::vector<VkSemaphore> imageAvailableSemaphores;
  std::vector<VkSemaphore> renderFinishedSemaphores;
  //栅栏
  std::vector<VkFence> inFlightFences;

  bool framebufferResized = false;

  VkBuffer vertexBuffer;
  VkBuffer indexBuffer;

  VkDeviceMemory vertexBufferMemory;
  VkDeviceMemory indexBufferMemory;

  VkDescriptorSetLayout descriptorSetLayout;
  
  std::vector<VkBuffer> uniformBuffers;
  std::vector<VkDeviceMemory> uniformBuffersMemory;
  std::vector<void*> uniformBuffersMapped;
  uint32_t currentFrame = 0;

  int frameCount = 0;
  double lastTime ;
  double currentTime;

  VkDescriptorPool descriptorPool;
  std::vector<VkDescriptorSet> descriptorSets;
};
