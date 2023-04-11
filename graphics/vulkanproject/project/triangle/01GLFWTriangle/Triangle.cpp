#include "Triangle.h"
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <chrono>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

// 设置最大同时运行帧数
const int MAX_FRAMES_IN_FLIGHT = 2;

// 需要的验证层
const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"};

// 需要的设备扩展
const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME};

const bool enableValidationLayers = true;

const std::vector<Vertex> vertices = {
{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
{{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
};

const std::vector<uint16_t> indices = {
0, 1, 2, 2, 3, 0
};

// 创建调试信息
VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pDebugMessenger) {
  auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkCreateDebugUtilsMessengerEXT");
  if (func != nullptr) {
    return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
  } else {
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
}

// 销毁调试信息
void DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                   VkDebugUtilsMessengerEXT debugMessenger,
                                   const VkAllocationCallbacks* pAllocator) {
  auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkDestroyDebugUtilsMessengerEXT");
  if (func != nullptr) {
    func(instance, debugMessenger, pAllocator);
  }
}

void HelloTriangleApplication::run() {
  initWindow();
  initVulkan();
  mainLoop();
  cleanup();
}
// 初始化窗口
void HelloTriangleApplication::initWindow() {
  glfwInit();

  // 不创建OpenGL上下文
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  // 创建窗口
  window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);

  // 设置窗口大小修改回调函数
  glfwSetWindowUserPointer(window, this);
  glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
}

void HelloTriangleApplication::framebufferResizeCallback(GLFWwindow* window, int width, int height){
  auto app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
  app->framebufferResized = true;
}

// 初始化Vulkan
void HelloTriangleApplication::initVulkan() {
  // 创建Vulkan实例
  createInstance();
  // 设置调试信息
  setupDebugMessenger();
  // 创建呈现表面
  createSurface();
  // 选择物理设备
  pickPhysicalDevice();
  // 创建逻辑设备
  createLogicalDevice();
  // 创建交换链
  createSwapChain();
  // 创建交换链中的图像视图
  createImageViews();
  // 创建渲染通道
  createRenderPass();
  // 创建描述集布局
  createDescriptorSetLayout();
  // 创建图形管线
  createGraphicsPipeline();
  // 创建帧缓冲
  createFramebuffers();
  // 创建命令池
  createCommandPool();
  // 创建顶点缓冲
  createVertexBuffer();
  // 创建索引缓冲
  createIndexBuffer();
  // 创建统一缓冲
  createUniformBuffers();
  // 创建描述池
  createDescriptorPool();
  // 创建描述集
  createDescriptorSets();
  // 创建命令缓冲
  createCommandBuffer();
  // 创建信号量
  createSyncObjects();
}

void HelloTriangleApplication::mainLoop() {
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    drawFrame();
  }

  vkDeviceWaitIdle(device);
}

void HelloTriangleApplication::cleanup() {
  cleanupSwapChain();

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
      vkDestroyBuffer(device, uniformBuffers[i], nullptr);
      vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
  }

  vkDestroyDescriptorPool(device, descriptorPool, nullptr);
  vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

  vkDestroyBuffer(device, vertexBuffer, nullptr);
  vkFreeMemory(device, vertexBufferMemory, nullptr);

  vkDestroyBuffer(device, indexBuffer, nullptr);
  vkFreeMemory(device, indexBufferMemory, nullptr);

  vkDestroyPipeline(device, graphicsPipeline, nullptr);
  vkDestroyPipelineLayout(device, pipelineLayout, nullptr);

  vkDestroyRenderPass(device, renderPass, nullptr);

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
      vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
      vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
      vkDestroyFence(device, inFlightFences[i], nullptr);
  }

  vkDestroyCommandPool(device, commandPool, nullptr);

  vkDestroyDevice(device, nullptr);

  if (enableValidationLayers) {
    DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
  }

  vkDestroySurfaceKHR(instance, surface, nullptr);
  vkDestroyInstance(instance, nullptr);

  glfwDestroyWindow(window);

  glfwTerminate();
}

void HelloTriangleApplication::cleanupSwapChain() {
  for (auto framebuffer : swapChainFramebuffers) {
    vkDestroyFramebuffer(device, framebuffer, nullptr);
  }

  for (auto imageView : swapChainImageViews) {
    vkDestroyImageView(device, imageView, nullptr);
  }

  vkDestroySwapchainKHR(device, swapChain, nullptr);
}

void HelloTriangleApplication::createInstance() {
  // 检查是否有可用的验证层
  if (!checkValidationLayerSupport()) {
    throw std::runtime_error("validation layers requested, but not available!");
  }

  // 应用程序信息
  VkApplicationInfo appInfo{};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = "Hello Triangle";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "No Engine";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  // Vulkan API版本
  appInfo.apiVersion = VK_API_VERSION_1_0;

  // 实例信息
  VkInstanceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;

  // 获取并启用扩展信息
  auto extensions = getRequiredExtensions();
  createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  createInfo.ppEnabledExtensionNames = extensions.data();

  // 启用验证层
  VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
  createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
  createInfo.ppEnabledLayerNames = validationLayers.data();

  populateDebugMessengerCreateInfo(debugCreateInfo);
  createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;

  // 创建Vulkan实例
  if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
    throw std::runtime_error("failed to create instance!");
  }
}

void HelloTriangleApplication::populateDebugMessengerCreateInfo(
    VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
  createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  createInfo.pfnUserCallback = debugCallback;
}

void HelloTriangleApplication::setupDebugMessenger() {
  if (!enableValidationLayers)
    return;

  VkDebugUtilsMessengerCreateInfoEXT createInfo;
  populateDebugMessengerCreateInfo(createInfo);

  if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr,
                                   &debugMessenger) != VK_SUCCESS) {
    throw std::runtime_error("failed to set up debug messenger!");
  }
}

void HelloTriangleApplication::createSurface() {
  // 使用GLFW创建呈现表面
  if (glfwCreateWindowSurface(instance, window, nullptr, &surface) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create window surface!");
  }
}

void HelloTriangleApplication::pickPhysicalDevice() {
  // 获取可用的物理设备数量
  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

  if (deviceCount == 0) {
    throw std::runtime_error("failed to find GPUs with Vulkan support!");
  }

  // 加载可用的物理设备
  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

  // 选择合适的物理设备
  for (const auto& device : devices) {
    if (isDeviceSuitable(device)) {
      physicalDevice = device;
      break;
    }
  }

  if (physicalDevice == VK_NULL_HANDLE) {
    throw std::runtime_error("failed to find a suitable GPU!");
  }
}

void HelloTriangleApplication::createLogicalDevice() {
  //    为选定的物理设备（例如 GPU）创建一个逻辑设备。逻辑设备是一个抽象，
  // 它表示物理设备上的资源和操作的抽象。创建逻辑设备后，可以使用它来分配内
  // 存、创建管道等

  //    为了使用逻辑设备，必须指定要使用的队列族。队列族是一组队列，它们
  // 在同一时间执行相同类型的操作。例如，图形队列族通常用于执行图形操作，
  // 而计算队列族通常用于执行计算操作。在 Vulkan 中，队列族是设备级别的概念，
  // 因此必须在逻辑设备创建时指定。在创建逻辑设备时，必须指定要使用的队列族
  // 的数量和优先级。优先级用于确定在同一队列族中的多个队列之间分配工作的方式。

  //    获取队列族(用于图形和呈现操作)
  QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

  // 存储队列创建信息结构
  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  // 存储唯一的队列族索引,不是顺序存储，是单个存储
  std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(),
                                            indices.presentFamily.value()};

  // 定义队列优先级
  float queuePriority = 1.0f;
  //    遍历 uniqueQueueFamilies 集合中的每个队列族索引，然后为每个索引创建
  // 一个 VkDeviceQueueCreateInfo 结构并将其添加到 queueCreateInfos 向量中
  for (uint32_t queueFamily : uniqueQueueFamilies) {
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = queueFamily;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;
    queueCreateInfos.push_back(queueCreateInfo);
  }

  //    创建一个 VkPhysicalDeviceFeatures 结构，用于指定所需的物理设备特性。在此
  // 示例中，我们没有启用任何特定特性。
  VkPhysicalDeviceFeatures deviceFeatures{};

  //    创建逻辑设备
  VkDeviceCreateInfo createInfo{};
  // 设置结构类型。
  createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

  // 设置队列创建信息结构的数量和数据指针。
  createInfo.queueCreateInfoCount =
      static_cast<uint32_t>(queueCreateInfos.size());
  createInfo.pQueueCreateInfos = queueCreateInfos.data();

  // 设置启用的设备特性指针。
  createInfo.pEnabledFeatures = &deviceFeatures;

  // 设置启用的设备扩展数量和名称。
  createInfo.enabledExtensionCount =
      static_cast<uint32_t>(deviceExtensions.size());
  createInfo.ppEnabledExtensionNames = deviceExtensions.data();

  // 设置启用的验证层数量和名称
  createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
  createInfo.ppEnabledLayerNames = validationLayers.data();

  //    创建逻辑设备
  if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create logical device!");
  }

  // 获取图形队列句柄，并将其存储在 graphicsQueue 变量中。
  vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
  // 获取显示队列句柄，并将其存储在 presentQueue 变量中。
  vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
}

void HelloTriangleApplication::createSwapChain() {
  //    创建一个交换链。交换链（Swap Chain）是一个包含多个图像的队列，
  // 这些图像在屏幕上交替显示以实现图形渲染。交换链的作用是处理图像的双
  // 缓冲（双缓冲技术可以使得渲染过程在一个图像上进行，而显示过程在另一
  // 个图像上进行，从而减少闪烁和撕裂现象）。

  // 查询交换链支持的详细信息
  SwapChainSupportDetails swapChainSupport =
      querySwapChainSupport(physicalDevice);

  // 选择交换链的图像格式、颜色空间和呈现模式
  VkSurfaceFormatKHR surfaceFormat =
      chooseSwapSurfaceFormat(swapChainSupport.formats);
  VkPresentModeKHR presentMode =
      chooseSwapPresentMode(swapChainSupport.presentModes);
  VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

  // 计算交换链中的图像数量。如果超过物理设备支持的最大图像数量，则减小图像数量。
  uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
  if (swapChainSupport.capabilities.maxImageCount > 0 &&
      imageCount > swapChainSupport.capabilities.maxImageCount) {
    imageCount = swapChainSupport.capabilities.maxImageCount;
  }

  // 创建交换链   填充交换链创建信息结构体（VkSwapchainCreateInfoKHR），
  // 包括表面、图像数量、表面格式、颜色空间、图像范围、图像使用方式等。
  VkSwapchainCreateInfoKHR createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.surface = surface;

  createInfo.minImageCount = imageCount;
  createInfo.imageFormat = surfaceFormat.format;
  createInfo.imageColorSpace = surfaceFormat.colorSpace;
  createInfo.imageExtent = extent;
  createInfo.imageArrayLayers = 1;
  createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
  uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(),
                                   indices.presentFamily.value()};

  if (indices.graphicsFamily != indices.presentFamily) {
    createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices = queueFamilyIndices;
  } else {
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  }

  createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  createInfo.presentMode = presentMode;
  createInfo.clipped = VK_TRUE;

  createInfo.oldSwapchain = VK_NULL_HANDLE;

  if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create swap chain!");
  }

  // 获取交换链中的图像，并将其存储在 swapChainImages 变量中。
  vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
  swapChainImages.resize(imageCount);
  vkGetSwapchainImagesKHR(device, swapChain, &imageCount,
                          swapChainImages.data());

  // 保存交换链的图像格式和图像范围。
  swapChainImageFormat = surfaceFormat.format;
  swapChainExtent = extent;
}

void HelloTriangleApplication::createImageViews() {
  //    创建ImageViews，用于访问交换链中的每个图像。这是Vulkan
  // 渲染过程的一部分，用于将渲染结果显示在屏幕上。

  // 调整交换链图像数组的大小，以便可以存储所有图像的句柄。
  swapChainImageViews.resize(swapChainImages.size());

  // 遍历交换链图像数组，为每个图像创建一个图像视图。
  for (size_t i = 0; i < swapChainImages.size(); i++) {
    //    为每个图像创建一个VkImageViewCreateInfo结构，用于描述ImageView的
    // 配置。这包括指定图像、视图类型（2D）、格式、颜色组件映射以及子资源
    // 范围（在本例中，是一个单一的Mipmap层级和一个数组层）。
    VkImageViewCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image = swapChainImages[i];
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = swapChainImageFormat;
    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;

    //    调用vkCreateImageView函数，使用提供的VkImageViewCreateInfo结构创建一个
    // 新的ImageView。将新创建的ImageView存储在swapChainImageViews数组中。
    if (vkCreateImageView(device, &createInfo, nullptr,
                          &swapChainImageViews[i]) != VK_SUCCESS) {
      throw std::runtime_error("failed to create image views!");
    }
  }
}

void HelloTriangleApplication::createRenderPass() {
  // 在Vulkan中，渲染通道（render pass）是一个重要的概念，用于描述图形管线在
  //渲染过程中所涉及的附件（如颜色、深度和模板缓冲区）以及这些附件如何使用的
  //信息。它表示一系列渲染操作，这些操作读取和写入一组图像资源，并最终生成一
  //个或多个渲染目标。

  //渲染通道的主要作用和意义如下：

  //性能优化：通过在渲染通道中描述所有渲染操作及其顺序，Vulkan可以在执行渲染
  //操作时对这些操作进行优化。这使得GPU能够更有效地使用其内部资源，提高性能。

  //资源管理：渲染通道清晰地定义了所有附件在渲染过程中的使用方式。这使得Vulkan
  //能够更好地管理这些资源，并确保它们在正确的时间被正确地使用。

  //模块化和可重用性：渲染通道将渲染操作分解为子通道，这使得可以将渲染过程划
  //分为逻辑上相关的阶段。每个子通道可以独立于其他子通道进行修改和优化，提高
  //了代码的模块化和可重用性。

  //附件使用和布局转换：渲染通道明确了附件在渲染过程中的状态转换。这有助于确保
  //所有渲染操作在正确的时间使用正确的附件布局，从而提高了性能。

  //总的来说，渲染通道为Vulkan提供了关于渲染操作的详细信息，使得Vulkan能够更有
  //效地管理和优化渲染过程。通过将渲染过程划分为子通道，渲染通道还提高了代码的
  //模块化和可重用性。

  //定义颜色附件的结构，包括格式、采样数、加载、存储和布局。
  VkAttachmentDescription colorAttachment{};
  colorAttachment.format = swapChainImageFormat;
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  //定义颜色附件的引用，包括附件的索引和布局，用于指定渲染通道中的哪个子通道将使用该附件。
  VkAttachmentReference colorAttachmentRef{};
  colorAttachmentRef.attachment = 0;
  colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  //定义子通道的结构，包括绑定点、颜色附件的引用和子通道的依赖关系。
  //子通道是渲染通道的一个阶段，用于处理附件的读写操作。
  VkSubpassDescription subpass{};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;

  //定义子通道之间的依赖关系。在这种情况下，依赖项表示从外部子通道到第一个（索引为0）子通道的转换。
  VkSubpassDependency dependency{};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  //定义渲染通道创建信息的结构，包括附件数量、附件描述、子通道数量等。
  VkRenderPassCreateInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = 1;
  renderPassInfo.pAttachments = &colorAttachment;
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;
  renderPassInfo.dependencyCount = 1;
  renderPassInfo.pDependencies = &dependency;

  //使用上述信息创建一个渲染通道，并将结果存储在renderPass变量中。
  if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create render pass!");
  }
}

void HelloTriangleApplication::createGraphicsPipeline() {
  //    创建图形管线（Graphics Pipeline）。在Vulkan中，图形管线是一个用于定义如何
  //进行渲染操作的对象。它包含了诸多渲染过程中涉及的状态和配置信息，例如着色器模块、
  //视口状态、光栅化状态、颜色混合状态等。
  auto vertShaderCode = readFile("shader/triangle.vert.spv");
  auto fragShaderCode = readFile("shader/triangle.frag.spv");

  //  使用着色器代码创建着色器模块（Shader Module）。
  VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
  VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

  auto bindingDescription = Vertex::getBindingDescription();
  auto attributeDescriptions = Vertex::getAttributeDescriptions();

  //为顶点和片段着色器设置管线阶段信息。
  VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
  vertShaderStageInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertShaderStageInfo.module = vertShaderModule;
  vertShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
  fragShaderStageInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragShaderStageInfo.module = fragShaderModule;
  fragShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo,
                                                    fragShaderStageInfo};

  //设置顶点的输入状态
  VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
  vertexInputInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  // 指定使用的绑定描述数量，即描述从哪个绑定获取顶点数据，此处为1，即只有一个绑定。
  vertexInputInfo.vertexBindingDescriptionCount = 1;
  // 指向 VkVertexInputBindingDescription
  // 结构的指针，描述顶点缓冲区中每个顶点数据的属性，此处为 bindingDescription。
  vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
  // 指定使用的顶点属性描述数量，即描述顶点属性数据类型和如何从缓冲区中获取属性数据的信息。
  vertexInputInfo.vertexAttributeDescriptionCount =
      static_cast<uint32_t>(attributeDescriptions.size());
  // 指向 VkVertexInputAttributeDescription
  // 结构数组的指针，每个结构体描述一个顶点属性，包括它们的位置、格式和偏移量，
  // 此处为 attributeDescriptions.data()，即获取 Vertex
  // 结构体的属性描述数组的指针。
  vertexInputInfo.pVertexAttributeDescriptions =
      attributeDescriptions.data();

  //设置输入装配状态
  VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
  inputAssembly.sType =
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  //设置视口状态
  VkPipelineViewportStateCreateInfo viewportState{};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.scissorCount = 1;

  //设置光栅化状态
  VkPipelineRasterizationStateCreateInfo rasterizer{};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth = 1.0f;
  rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rasterizer.depthBiasEnable = VK_FALSE;

  //设置多重采样状态
  VkPipelineMultisampleStateCreateInfo multisampling{};
  multisampling.sType =
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  //设置颜色混合状态
  VkPipelineColorBlendAttachmentState colorBlendAttachment{};
  colorBlendAttachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_FALSE;

  VkPipelineColorBlendStateCreateInfo colorBlending{};
  colorBlending.sType =
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.logicOpEnable = VK_FALSE;
  colorBlending.logicOp = VK_LOGIC_OP_COPY;
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorBlendAttachment;
  colorBlending.blendConstants[0] = 0.0f;
  colorBlending.blendConstants[1] = 0.0f;
  colorBlending.blendConstants[2] = 0.0f;
  colorBlending.blendConstants[3] = 0.0f;

  std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT,
                                               VK_DYNAMIC_STATE_SCISSOR};
  VkPipelineDynamicStateCreateInfo dynamicState{};
  dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
  dynamicState.pDynamicStates = dynamicStates.data();

  //设置管线布局，它描述着色器使用的资源布局，例如描述符集、推送常量等。
  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 1;
  pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

  if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr,
                             &pipelineLayout) != VK_SUCCESS) {
    throw std::runtime_error("failed to create pipeline layout!");
  }

  //    创建图形管线，这一步将前面准备好的各种状态和信息整合在一起。同时指定
  //管线所属的渲染通道（Render Pass）和子通道（Subpass）。
  VkGraphicsPipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount = 2;
  pipelineInfo.pStages = shaderStages;
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.pDynamicState = &dynamicState;
  pipelineInfo.layout = pipelineLayout;
  pipelineInfo.renderPass = renderPass;
  pipelineInfo.subpass = 0;
  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

  if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo,
                                nullptr, &graphicsPipeline) != VK_SUCCESS) {
    throw std::runtime_error("failed to create graphics pipeline!");
  }

  vkDestroyShaderModule(device, fragShaderModule, nullptr);
  vkDestroyShaderModule(device, vertShaderModule, nullptr);
}

void HelloTriangleApplication::createFramebuffers() {
  //    为交换链中的每个图像创建一个帧缓冲对象。帧缓冲对象将渲染过程中
  //生成的图像与渲染通道（Render Pass）关联起来，以便在渲染过程中使用。
  swapChainFramebuffers.resize(swapChainImageViews.size());

  //遍历交换链中的每个图像视图，为其创建帧缓冲对象。
  for (size_t i = 0; i < swapChainImageViews.size(); i++) {
    //将当前图像视图添加到帧缓冲对象的附件列表中。
    VkImageView attachments[] = {swapChainImageViews[i]};

    //  创建一个VkFramebufferCreateInfo结构，并填充相关信息，包括渲染通
    //道、附件数量和类型（在本例中为交换链图像视图）、帧缓冲的宽度和高度以
    //及层数。
    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = attachments;
    framebufferInfo.width = swapChainExtent.width;
    framebufferInfo.height = swapChainExtent.height;
    framebufferInfo.layers = 1;

    //使用vkCreateFramebuffer函数创建帧缓冲对象，并将其存储在swapChainFramebuffers数组中。
    if (vkCreateFramebuffer(device, &framebufferInfo, nullptr,
                            &swapChainFramebuffers[i]) != VK_SUCCESS) {
      throw std::runtime_error("failed to create framebuffer!");
    }
  }
}

void HelloTriangleApplication::createCommandPool() {
  //    在 Vulkan 应用程序中创建一个命令池。命令池是一个存储命令缓冲区的容器，这些命令缓冲区
  //用于记录渲染和计算操作。在这个例子中，我们正在创建一个与图形队列族关联的命令池。

  //    命令池在Vulkan中具有重要作用，它们主要用于管理和分配命令缓冲区。命令缓冲区用于存储要
  //在GPU上执行的命令序列，例如绘制操作、缓冲区更新、图像布局转换等。在Vulkan中，许多操作都
  //通过命令缓冲区完成，因此命令池在整个渲染过程中扮演着关键角色。

  //    命令池的主要功能和含义如下：
  //  分配命令缓冲区：命令池负责为命令缓冲区分配内存。当需要一个新的命令缓冲区时，可以从命令
  //池中分配一个。这种方法允许Vulkan实现对内存的高效管理，从而提高性能。
  //  与特定队列族关联：每个命令池都与特定的队列族关联。这意味着从该命令池分配的命令缓冲区只
  //能在与之关联的队列族中提交执行。这样做有助于确保正确的内存类型和访问模式，从而提高性能。
  //  重置命令缓冲区：命令池允许在不重新分配内存的情况下重置其关联的命令缓冲区，这有助于减少
  //内存分配和释放操作的开销，提高效率。通过重置命令缓冲区，我们可以在后续帧中重用它们，而无
  //需为每一帧创建新的命令缓冲区。
  //  线程局部存储：命令池可以为每个线程分配独立的命令缓冲区，从而实现多线程渲染。这样可以确
  //保多个线程同时处理渲染任务，而不会相互干扰，从而提高渲染速度。
  
  //    总之，命令池在Vulkan中主要负责分配、管理和重置命令缓冲区，与特定队列族关联，并支持多
  //线程渲染。这些功能有助于提高渲染性能、降低内存管理开销，并确保正确的资源访问模式。
  
  QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

  //定义一个结构，用于指定创建命令池所需的参数。
  VkCommandPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  //设置命令池的创建标志。在这里，我们使用 VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT，
  //以允许单个命令缓冲区在已经提交给队列之后被重置，而不是在整个命令池被重置时才能重置。
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  //设置命令池所关联的队列族索引。在这个例子中，我们使用图形队列族的索引。
  poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

  //创建命令池。
  if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create command pool!");
  }
}


void HelloTriangleApplication::createVertexBuffer(){
  //    顶点缓冲区是一块内存，用于存储顶点数据。在这个例子中，我们将使用它来存储三角形的顶点
  //数据。顶点缓冲区的创建过程如下：
 
  VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               stagingBuffer, stagingBufferMemory);

  //    4.将顶点数据复制到缓冲区中。
  void *data;
  //将缓冲区的内存映射到CPU可访问的内存中。
  //这个函数允许我们访问由偏移量和大小定义的指定内存资源的一部分。这里的偏移和大小分别为0和bufferInfo.size。
  //也可以指定特殊值VK_WHOLE_SIZE来映射所有内存。倒数第二个参数可以用于指定标志，但是在当前的API中还没有可
  // 用的标志。必须将其设置为值0。最后一个参数指定映射内存的指针的输出。
  vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
  // 将顶点数据复制到缓冲区中。
  memcpy(data, vertices.data(), (size_t)bufferSize);

  //现在只需将顶点数据复制到映射内存中，并使用vkUnmapMemory再次取消映射。不幸的是，驱动程序可能不会立即将数
  //据复制到缓冲区内存中，例如由于缓存。还可能写入缓冲区的内容尚未在映射内存中可见。有两种方法可以解决这个问题：
  //    1.使用主机一致性内存堆，使用VK_MEMORY_PROPERTY_HOST_COHERENT_BIT进行标识
  //    2.在写入映射内存后调用vkFlushMappedMemoryRanges，并在从映射内存中读取之前调用vkInvalidateMappedMemoryRanges
  //我们采用了第一种方法，这可以确保映射内存始终与分配的内存内容相匹配。请记住，这可能导致稍微较差的性能比显式刷新，
  //但我们将在下一章中看到为什么这并不重要。

  //刷新内存范围或使用一致性内存堆意味着驱动程序将知道我们对缓冲区的写入，但这并不意味着它们已经在GPU上可见。
  //数据传输到GPU是在后台完成的操作，规范只告诉我们，它保证在下一次调用vkQueueSubmit时完成。
  // 取消内存映射。
  vkUnmapMemory(device, stagingBufferMemory);

  //我们将使用两个新的缓冲区用途标志：
  //  VK_BUFFER_USAGE_TRANSFER_SRC_BIT：缓冲区可用作内存传输操作的源。
  //  VK_BUFFER_USAGE_TRANSFER_DST_BIT：缓冲区可用作内存传输操作的目标。
  createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
                         , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);

  // 从暂存缓冲区复制顶点数据到设备缓冲区
  copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

  // 释放暂存缓冲区
  vkDestroyBuffer(device, stagingBuffer, nullptr);
  vkFreeMemory(device, stagingBufferMemory, nullptr);
}


void HelloTriangleApplication::createIndexBuffer(){
  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  createBuffer(sizeof(indices[0]) * indices.size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               stagingBuffer, stagingBufferMemory);
  
  void *data;
  vkMapMemory(device, stagingBufferMemory, 0, sizeof(indices[0]) * indices.size(), 0, &data);
  memcpy(data, indices.data(), (size_t)(sizeof(indices[0]) * indices.size()));
  vkUnmapMemory(device, stagingBufferMemory);

  createBuffer(sizeof(indices[0]) * indices.size(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT
                         , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);
  
  copyBuffer(stagingBuffer, indexBuffer, sizeof(indices[0]) * indices.size());

  vkDestroyBuffer(device, stagingBuffer, nullptr);
  vkFreeMemory(device, stagingBufferMemory, nullptr);
}

 void HelloTriangleApplication::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = commandPool;
  allocInfo.commandBufferCount = 1;

  VkCommandBuffer commandBuffer;
  vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(commandBuffer, &beginInfo);

  VkBufferCopy copyRegion{};
  copyRegion.size = size;
  vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

  vkEndCommandBuffer(commandBuffer);

  //    5.将命令缓冲区提交到队列中。
  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(graphicsQueue);


  vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
 }

void HelloTriangleApplication::createDescriptorSetLayout() {
  VkDescriptorSetLayoutBinding uboLayoutBinding{};
  //   1.绑定点：这是一个32位无符号整数，用于标识绑定的描述符。这个值可以用来访问描述符数组中的元素。
  uboLayoutBinding.binding = 0;
  //   2.描述符类型：这是一个VkDescriptorType值，用于指定描述符的类型。
  uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  //   3.描述符数量：这是一个无符号整数，指定了描述符数组中的描述符数量。如果我们想要使用一个描述符数组，
  uboLayoutBinding.descriptorCount = 1;

  //   4.着色器阶段：这是一个VkShaderStageFlags掩码，指定了哪些着色器阶段可以访问描述符。
  //   我们可以使用VK_SHADER_STAGE_VERTEX_BIT和VK_SHADER_STAGE_FRAGMENT_BIT来指定顶点和片段着色器。
  uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  //   5.描述符绑定：这是一个VkSampler对象的数组，用于指定描述符中使用的纹理采样器。如果描述符不是一个纹理描述符，
  //   这个值应该被忽略。
  uboLayoutBinding.pImmutableSamplers = nullptr;

  VkDescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = 1;
  layoutInfo.pBindings = &uboLayoutBinding;

  if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr,
                                  &descriptorSetLayout) != VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor set layout!");
  }
}


void HelloTriangleApplication::createUniformBuffers() {
  VkDeviceSize bufferSize = sizeof(UniformBufferObject);

  uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
  uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
  uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 uniformBuffers[i], uniformBuffersMemory[i]);

    vkMapMemory(device, uniformBuffersMemory[i], 0, bufferSize, 0,
                &uniformBuffersMapped[i]);
  }
}

void HelloTriangleApplication::createDescriptorPool(){
  VkDescriptorPoolSize poolSize{};
  poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  //将为每一帧分配一个描述符
  poolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = 1;
  poolInfo.pPoolSizes = &poolSize;
  poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

  if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor pool!");
  }
}

void HelloTriangleApplication::createDescriptorSets(){
  std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = descriptorPool;
  allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
  allocInfo.pSetLayouts = layouts.data();

  descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
  if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate descriptor sets!");
  }

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = uniformBuffers[i];
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UniformBufferObject);

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = descriptorSets[i];
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pBufferInfo = &bufferInfo;
    descriptorWrite.pImageInfo = nullptr; // Optional
    descriptorWrite.pTexelBufferView = nullptr; // Optional

    vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
  }

}

void HelloTriangleApplication::updateUniformBuffer(uint32_t currentImage) {
  static auto startTime = std::chrono::high_resolution_clock::now();
  auto currentTime = std::chrono::high_resolution_clock::now();
  float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

  UniformBufferObject ubo{};
  ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
  ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                         glm::vec3(0.0f, 0.0f, 1.0f));
  ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 10.0f);
  //    由于我们使用的是右手坐标系，因此需要将Y轴翻转。
  ubo.proj[1][1] *= -1;

  //将uniform buffer object中的数据复制到当前的uniform buffer中
  memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}

uint32_t HelloTriangleApplication::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties){
  //    在Vulkan中，内存是通过VkDeviceMemory对象来表示的。这些对象并不直接代表物理内存，而
  //是代表了一段可以由应用程序访问的内存。内存对象可以从物理设备的内存中分配，也可以从外部
  //源（如文件）导入。内存对象的大小是有限的，因此必须在使用之前将其分配给某个缓冲区或图像。
  //内存对象的分配和释放是昂贵的操作，因此应该尽可能地重用它们。为了实现这一点，Vulkan提供
  //了一种名为“绑定内存”的机制，用于将内存对象与缓冲区或图像相关联。在这个例子中，我们将
  //使用它来将内存对象与顶点缓冲区相关联。

  //查询物理设备的内存属性。
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

  //遍历所有的内存类型，找到符合要求的内存类型。
  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
    //如果当前内存类型符合要求。
    if ((typeFilter & (1 << i)) &&
        (memProperties.memoryTypes[i].propertyFlags & properties) ==
            properties) {
      //返回内存类型的索引。
      return i;
    }
  }
  throw std::runtime_error("failed to find suitable memory type!");
}

void HelloTriangleApplication::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                  VkMemoryPropertyFlags properties, VkBuffer& buffer,
                  VkDeviceMemory& bufferMemory){
  //    1.创建缓冲区。
  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = size;
  bufferInfo.usage = usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
    throw std::runtime_error("failed to create buffer!");
  }
  //    2.分配缓冲区内存。
  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

  if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate buffer memory!");
  }
  //    3.将缓冲区与内存绑定。 
  vkBindBufferMemory(device, buffer, bufferMemory, 0); 
}

void HelloTriangleApplication::createCommandBuffer() {
  commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
  //  命令缓冲区用于记录将要提交到设备队列执行的命令。这段代码中的函数创建了一个
  //主要级别（primary level）的命令缓冲区。

  //初始化 VkCommandBufferAllocateInfo 结构，它用于指定命令缓冲区的分配信息。
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  //设置命令池。在这个例子中，我们使用的是之前创建的命令池。
  allocInfo.commandPool = commandPool;
  //设置命令缓冲区的级别。在这个例子中，我们使用的是主要级别的命令缓冲区。
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  //设置命令缓冲区的数量。在这个例子中，我们只需要一个命令缓冲区。
  allocInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

  if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to allocate command buffers!");
  }
}

void HelloTriangleApplication::recordCommandBuffer(
    VkCommandBuffer commandBuffer,
    uint32_t imageIndex) {
  //    命令缓冲区包含了绘制一个三角形所需的所有命令。该函数接受两个参数：一个 VkCommandBuffer 
  //对象和一个 imageIndex，分别表示要记录的命令缓冲区和要绘制到的交换链图像的索引。
  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
    throw std::runtime_error("failed to begin recording command buffer!");
  }

  //  初始化 VkRenderPassBeginInfo 结构，用于指定渲染通道的开始信息，包括渲染通道对象、
  //帧缓冲区、渲染区域等。
  VkRenderPassBeginInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = renderPass;
  renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = swapChainExtent;

  //设置清除颜色
  VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
  renderPassInfo.clearValueCount = 1;
  renderPassInfo.pClearValues = &clearColor;

  //使用 vkCmdBeginRenderPass 命令开始渲染通道。
  vkCmdBeginRenderPass(commandBuffer, &renderPassInfo,
                       VK_SUBPASS_CONTENTS_INLINE);

  //使用 vkCmdBindPipeline 命令绑定之前创建的图形管线。
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    graphicsPipeline);

  //vkCmdBindVertexBuffers 函数用于将顶点缓冲区绑定到绑定点上.除了命令缓冲区
  //之外，前两个参数指定要为其指定顶点缓冲区的偏移量和绑定数。最后两个参数指定
  // 要绑定的顶点缓冲区数组以及开始读取顶点数据的字节偏移量。
  VkBuffer vertexBuffers[] = {vertexBuffer};
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
  //使用 vkCmdBindIndexBuffer 命令将索引缓冲区绑定到绑定点上。
  vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);


  //设置视口（VkViewport）并使用 vkCmdSetViewport 命令将其应用到命令缓冲区。
  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = (float)swapChainExtent.width;
  viewport.height = (float)swapChainExtent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

  //设置剪裁矩形（VkRect2D），并使用 vkCmdSetScissor 命令将其应用到命令缓冲区。
  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = swapChainExtent;
  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

  
  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);

  //前两个参数指定索引数量和实例数量。我们不使用实例，因此只需指定1个实例。索引
  //数表示将传递给顶点着色器的顶点数。下一个参数指定索引缓冲区中的偏移量，使用
  //值1会导致图形卡从第二个索引开始读取。倒数第二个参数指定添加到索引缓冲区中的
  //索引的偏移量。最后一个参数指定实例的偏移量，我们不使用它。
  vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

  //使用 vkCmdEndRenderPass 命令结束渲染通道。
  vkCmdEndRenderPass(commandBuffer);

  //使用 vkEndCommandBuffer 函数结束命令缓冲区的记录。
  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
    throw std::runtime_error("failed to record command buffer!");
  }
}

void HelloTriangleApplication::createSyncObjects() {
  imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
  //  创建同步对象，包括信号量（semaphores）和栅栏（fences）。在 Vulkan 中，同步对象
  //主要用于确保渲染操作按照正确的顺序执行，以及避免资源冲突和竞争条件。
  VkSemaphoreCreateInfo semaphoreInfo{};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  //  imageAvailableSemaphore：这个信号量用于确保图像在提交给渲染队列之前已经准备好了。
  //它通常用于同步图像的获取操作和渲染操作之间的顺序。

  //  renderFinishedSemaphore：这个信号量用于确保渲染操作完成后，图像才能进行显示。它通
  //常用于同步渲染操作和图像显示操作之间的顺序。

  //  inFlightFence：这个栅栏用于确保当前帧的渲染操作已经完成，才能开始下一帧的渲染。
  //栅栏主要用于同步 CPU 和 GPU 之间的操作。

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
      if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
          vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
          vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
          throw std::runtime_error("failed to create synchronization objects for a frame!");
      }
  }
}

void HelloTriangleApplication::drawFrame() {
  vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
  // 该函数会执行渲染过程的各个阶段，并确保它们正确地同步。

  //    等待之前提交的渲染操作完成。这里使用 vkWaitForFences 函数等待栅栏
  //信号，以确保 GPU 不会在前一个帧的渲染操作仍在进行时开始新的渲染操作。
  vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE,
                  UINT64_MAX);
  uint32_t imageIndex;
  //   使用 vkAcquireNextImageKHR 函数获取交换链中的下一个可用图像。该函数
  //使用 imageAvailableSemaphore 信号量来确保图像准备就绪。
  VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, 
                      imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
  //    如果窗口大小发生变化，交换链将变得无效。在这种情况下，需要重新创建交换链。
  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    reCreateSwapChain();
    return;
  } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    //    如果 vkAcquireNextImageKHR 函数返回 VK_ERROR_OUT_OF_DATE_KHR 错误，
    //则交换链已经过时，需要重新创建交换链。如果返回 VK_SUBOPTIMAL_KHR 错误，
    //则交换链仍然可用，但是不再完全符合预期。
    throw std::runtime_error("failed to acquire swap chain image!");
  }
  
  //更新 Uniform 缓冲区
  updateUniformBuffer(currentFrame);

  vkResetFences(device, 1, &inFlightFences[currentFrame]);

  //    重置命令缓冲区，以便重新记录渲染命令。然后使用 recordCommandBuffer
  //函数记录渲染命令。
  vkResetCommandBuffer(commandBuffers[currentFrame],
                       /*VkCommandBufferResetFlagBits*/ 0);
  recordCommandBuffer(commandBuffers[currentFrame], imageIndex);


  //    使用 vkQueueSubmit 函数将渲染命令提交给图形队列。这个函数需要指定等
  //待信号量（imageAvailableSemaphore）、等待阶段
  //（VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT）、命令缓冲区和信号信
  //号量（renderFinishedSemaphore）。
  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
  VkPipelineStageFlags waitStages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;

  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

  VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores;

  if (vkQueueSubmit(graphicsQueue, 1, &submitInfo,
                    inFlightFences[currentFrame]) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to submit draw command buffer!");
  }

  //    在渲染完成后，使用 vkQueuePresentKHR 函数将渲染结果显示到屏幕上。
  //这个函数需要等待 renderFinishedSemaphore 信号量，以确保在 GPU 完成
  //渲染操作后才呈现结果。
  VkPresentInfoKHR presentInfo{};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = signalSemaphores;

  VkSwapchainKHR swapChains[] = {swapChain};
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapChains;

  presentInfo.pImageIndices = &imageIndex;

  result = vkQueuePresentKHR(presentQueue, &presentInfo);

  //    如果窗口大小发生变化，交换链将变得无效。在这种情况下，需要重新创建交换链。
  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
      framebufferResized) {
    framebufferResized = false;
    reCreateSwapChain();
  } else if (result != VK_SUCCESS) {
  //    如果 vkQueuePresentKHR 函数返回 VK_ERROR_OUT_OF_DATE_KHR 错误，
  //则交换链已经过时，需要重新创建交换链。如果返回 VK_SUBOPTIMAL_KHR 错误，
  //则交换链仍然可用，但是不再完全符合预期。
    throw std::runtime_error("failed to present swap chain image!");
  }
  currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void HelloTriangleApplication::reCreateSwapChain() {
  int width = 0, height = 0;
  glfwGetFramebufferSize(window, &width, &height);
  while (width == 0 || height == 0) {
    glfwGetFramebufferSize(window, &width, &height);
    glfwWaitEvents();
  }

  // 1. 等待设备空闲
  vkDeviceWaitIdle(device);

  // 2. 释放交换链相关资源
  cleanupSwapChain();

  // 3. 重新创建交换链
  //我们重新创建了交换
  //链。图形视图是直接依赖于交换链图像的，所以也需要被重建图像视图。渲
  //染流程依赖于交换链图像的格式，虽然像窗口大小改变不会引起使用的交
  //换链图像格式改变，但我们还是应该对它进行处理。视口和裁剪矩形在管
  //线创建时被指定，窗口大小改变，这些设置也需要修改，所以我们也需要
  //重建管线。实际上，我们可以通过使用动态状态来设置视口和裁剪矩形来
  //避免重建管线。帧缓冲和指令缓冲直接依赖于交换链图像，也需要重建。
  createSwapChain();
  createImageViews();
  createFramebuffers();
}

VkShaderModule HelloTriangleApplication::createShaderModule(
    const std::vector<char>& code) {
  VkShaderModuleCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = code.size();
  createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

  VkShaderModule shaderModule;
  if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create shader module!");
  }

  return shaderModule;
}

VkSurfaceFormatKHR HelloTriangleApplication::chooseSwapSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR>& availableFormats) {
  for (const auto& availableFormat : availableFormats) {
    if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
        availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      return availableFormat;
    }
  }

  return availableFormats[0];
}

VkPresentModeKHR HelloTriangleApplication::chooseSwapPresentMode(
    const std::vector<VkPresentModeKHR>& availablePresentModes) {
  for (const auto& availablePresentMode : availablePresentModes) {
    if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
      return availablePresentMode;
    }
  }

  return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D HelloTriangleApplication::chooseSwapExtent(
    const VkSurfaceCapabilitiesKHR& capabilities) {
  if (capabilities.currentExtent.width != UINT32_MAX) {
    return capabilities.currentExtent;
  } else {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    VkExtent2D actualExtent = {static_cast<uint32_t>(width),
                               static_cast<uint32_t>(height)};

    actualExtent.width = std::max(
        capabilities.minImageExtent.width,
        std::min(capabilities.maxImageExtent.width, actualExtent.width));
    actualExtent.height = std::max(
        capabilities.minImageExtent.height,
        std::min(capabilities.maxImageExtent.height, actualExtent.height));

    return actualExtent;
  }
}

SwapChainSupportDetails HelloTriangleApplication::querySwapChainSupport(
    VkPhysicalDevice device) {
  //    查询给定物理设备（如 GPU）对交换链的支持。交换链是一种用于在
  // 渲染管道中高效显示图像的技术，通过在内存中的多个图像之间切换来提高性能和减少画面撕裂。

  //   交换链的支持包括以下内容：
  //    1.支持的像素格式
  //    2.支持的显示模式
  //    3.交换链图像的最大和最小尺寸
  SwapChainSupportDetails details;

  //   查询交换链的基本功能，这些能力包括最小/最大图像数量、图像宽度/高度范围等。
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface,
                                            &details.capabilities);

  //   查询表面格式的数量
  uint32_t formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

  // 如果表面格式的数量大于 0，那么就获取表面格式的详细信息。
  if (formatCount != 0) {
    details.formats.resize(formatCount);
    //   再次调用 vkGetPhysicalDeviceSurfaceFormatsKHR
    //   函数，以填充实际的表面格式数据。
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount,
                                         details.formats.data());
  }

  //   查询表面支持的显示模式的数量。
  uint32_t presentModeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount,
                                            nullptr);
  //  如果表面支持的显示模式的数量大于
  //  0，那么就获取表面支持的显示模式的详细信息。
  if (presentModeCount != 0) {
    details.presentModes.resize(presentModeCount);
    //   再次调用 vkGetPhysicalDeviceSurfacePresentModesKHR
    //   函数，以填充实际的表面支持的显示模式数据。
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        device, surface, &presentModeCount, details.presentModes.data());
  }

  //   返回查询到的交换链支持的详细信息。
  return details;
}

bool HelloTriangleApplication::isDeviceSuitable(VkPhysicalDevice device) {
  //    查询物理设备支持的队列族。队列族是执行特定类型操作的队列的集合，
  // 例如图形渲染或计算操作。这里获取的 indices 对象将包含设备上可用的
  // 图形和显示队列族的索引。
  QueueFamilyIndices indices = findQueueFamilies(device);

  //    检查设备扩展是否可用。这里检查的是设备扩展，而不是实例扩展。
  // 与检查实例层类似，我们需要列出我们需要的扩展并检查它们是否可用。
  bool extensionsSupported = checkDeviceExtensionSupport(device);

  //    检查交换链是否满足要求。我们需要确保交换链至少支持一个图像格式和一个呈现模式。
  bool swapChainAdequate = false;
  if (extensionsSupported) {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
    swapChainAdequate = !swapChainSupport.formats.empty() &&
                        !swapChainSupport.presentModes.empty();
  }

  //    检查设备是否满足我们的需求。这里我们检查的是设备是否具有足够的功能来运行我们的应用程序。
  // 我们需要确保设备至少支持一个图形队列族和一个呈现队列族。
  // 我们还需要确保设备支持交换链所需的扩展。
  // 最后，我们需要确保交换链支持至少一个图像格式和一个呈现模式。
  return indices.isComplete() && extensionsSupported && swapChainAdequate;
}

bool HelloTriangleApplication::checkDeviceExtensionSupport(
    VkPhysicalDevice device) {
  // 这个函数检查device是否支持我们需要的扩展。我们需要列出我们需要的扩展并检查它们是否可用。

  uint32_t extensionCount;
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
                                       nullptr);

  std::vector<VkExtensionProperties> availableExtensions(extensionCount);
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
                                       availableExtensions.data());

  std::set<std::string> requiredExtensions(deviceExtensions.begin(),
                                           deviceExtensions.end());

  for (const auto& extension : availableExtensions) {
    requiredExtensions.erase(extension.extensionName);
  }

  return requiredExtensions.empty();
}

QueueFamilyIndices HelloTriangleApplication::findQueueFamilies(
    VkPhysicalDevice device) {
  QueueFamilyIndices indices;

  //   获取物理设备的队列族数量
  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

  // 获取物理设备的队列族属性

  // VkQueueFamilyProperties 结构体。这个结构体包含了关于队列族的信息，例如：
  // 队列个数（queueCount）：队列族中的队列数量。
  // 队列功能（queueFlags）：表明队列支持的操作，如图形、计算和传输等。
  // 时间戳有效位宽度（timestampValidBits）：队列族支持的时间戳的有效位宽。
  // 图像传输操作限制（minImageTransferGranularity）：队列族支持的最小图像传输操作粒度。
  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
                                           queueFamilies.data());

  // 遍历队列族，找到支持VK_QUEUE_GRAPHICS_BIT的队列族
  int i = 0;
  for (const auto& queueFamily : queueFamilies) {
    if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      indices.graphicsFamily = i;
    }

    // 检查队列族是否支持present操作
    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

    if (presentSupport) {
      indices.presentFamily = i;
    }

    if (indices.isComplete()) {
      break;
    }

    i++;
  }

  return indices;
}

std::vector<const char*> HelloTriangleApplication::getRequiredExtensions() {
  uint32_t glfwExtensionCount = 0;
  const char** glfwExtensions;
  glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

  std::vector<const char*> extensions(glfwExtensions,
                                      glfwExtensions + glfwExtensionCount);

  if (enableValidationLayers) {
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }

  return extensions;
}

bool HelloTriangleApplication::checkValidationLayerSupport() {
  uint32_t layerCount;
  vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

  std::vector<VkLayerProperties> availableLayers(layerCount);
  vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

  for (const char* layerName : validationLayers) {
    bool layerFound = false;

    for (const auto& layerProperties : availableLayers) {
      if (strcmp(layerName, layerProperties.layerName) == 0) {
        layerFound = true;
        break;
      }
    }

    if (!layerFound) {
      return false;
    }
  }

  return true;
}

std::vector<char> HelloTriangleApplication::readFile(
    const std::string& filename) {
  std::ifstream file(filename, std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    throw std::runtime_error("failed to open file!");
  }

  size_t fileSize = (size_t)file.tellg();
  std::vector<char> buffer(fileSize);

  file.seekg(0);
  file.read(buffer.data(), fileSize);

  file.close();

  return buffer;
}

VkBool32 VKAPI_CALL HelloTriangleApplication::debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {
  std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

  return VK_FALSE;
}
