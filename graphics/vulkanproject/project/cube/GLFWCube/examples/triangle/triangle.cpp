﻿#include "triangle.h"

VulkanExample::VulkanExample() : VulkanExampleBase(ENABLE_VALIDATION) {
  title = "Vulkan Example - Basic indexed triangle";
  // To keep things simple, we don't use the UI overlay
  settings.overlay = false;
  // Setup a default look-at camera
  camera.type = Camera::CameraType::lookat;
  camera.setPosition(glm::vec3(0.0f, 0.0f, -4.5f));
  camera.setRotation(glm::vec3(0.0f));
  camera.setPerspective(60.0f, (float)width / (float)height, 1.0f, 256.0f);
  // Values not set here are initialized in the base class constructor
}

VulkanExample::~VulkanExample() { 
  // Clean up used Vulkan resources
  // Note: Inherited destructor cleans up resources stored in base class
  vkDestroyPipeline(device, pipeline, nullptr);

  vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
  vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

  vkDestroyBuffer(device, vertices.buffer, nullptr);
  vkFreeMemory(device, vertices.memory, nullptr);

  vkDestroyBuffer(device, indices.buffer, nullptr);
  vkFreeMemory(device, indices.memory, nullptr);

  vkDestroyBuffer(device, uniformBufferVS.buffer, nullptr);
  vkFreeMemory(device, uniformBufferVS.memory, nullptr);

  vkDestroySemaphore(device, presentCompleteSemaphore, nullptr);
  vkDestroySemaphore(device, renderCompleteSemaphore, nullptr);

  for (auto& fence : queueCompleteFences) {
    vkDestroyFence(device, fence, nullptr);
  }
}

// This function is used to request a device memory type that supports all the
// property flags we request (e.g. device local, host visible) Upon success it
// will return the index of the memory type that fits our requested memory
// properties This is necessary as implementations can offer an arbitrary
// number of memory types with different memory properties. You can check
// http://vulkan.gpuinfo.org/ for details on different memory configurations
uint32_t VulkanExample::getMemoryTypeIndex(uint32_t typeBits,
                                            VkMemoryPropertyFlags properties) {
  // Iterate over all memory types available for the device used
  // in this example
  for (uint32_t i = 0; i < deviceMemoryProperties.memoryTypeCount; i++) {
    if ((typeBits & 1) == 1) {
      if ((deviceMemoryProperties.memoryTypes[i].propertyFlags &
            properties) == properties) {
        return i;
      }
    }
    typeBits >>= 1;
  }

  throw "Could not find a suitable memory type!";
}

// Create the Vulkan synchronization primitives used in this example
void VulkanExample::prepareSynchronizationPrimitives() {
  // Semaphores (Used for correct command ordering)
  VkSemaphoreCreateInfo semaphoreCreateInfo = {};
  semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  semaphoreCreateInfo.pNext = nullptr;

  // Semaphore used to ensure that image presentation is complete before
  // starting to submit again
  VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr,
                                    &presentCompleteSemaphore));

  // Semaphore used to ensure that all commands submitted have been finished
  // before submitting the image to the queue
  VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr,
                                    &renderCompleteSemaphore));

  // Fences (Used to check draw command buffer completion)
  VkFenceCreateInfo fenceCreateInfo = {};
  fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  // Create in signaled state so we don't wait on first render of each command
  // buffer
  fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
  queueCompleteFences.resize(drawCmdBuffers.size());
  for (auto& fence : queueCompleteFences) {
    VK_CHECK_RESULT(vkCreateFence(device, &fenceCreateInfo, nullptr, &fence));
  }
}

// Get a new command buffer from the command pool
// If begin is true, the command buffer is also started so we can start adding
// commands
VkCommandBuffer VulkanExample::getCommandBuffer(bool begin) {
  VkCommandBuffer cmdBuffer;

  VkCommandBufferAllocateInfo cmdBufAllocateInfo = {};
  cmdBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  cmdBufAllocateInfo.commandPool = cmdPool;
  cmdBufAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  cmdBufAllocateInfo.commandBufferCount = 1;

  VK_CHECK_RESULT(
      vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, &cmdBuffer));

  // If requested, also start the new command buffer
  if (begin) {
    VkCommandBufferBeginInfo cmdBufInfo =
        vks::initializers::commandBufferBeginInfo();
    VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo));
  }

  return cmdBuffer;
}

// End the command buffer and submit it to the queue
// Uses a fence to ensure command buffer has finished executing before
// deleting it
void VulkanExample::flushCommandBuffer(VkCommandBuffer commandBuffer) {
  assert(commandBuffer != VK_NULL_HANDLE);

  VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));

  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  // Create fence to ensure that the command buffer has finished executing
  VkFenceCreateInfo fenceCreateInfo = {};
  fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceCreateInfo.flags = 0;
  VkFence fence;
  VK_CHECK_RESULT(vkCreateFence(device, &fenceCreateInfo, nullptr, &fence));

  // Submit to the queue
  VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, fence));
  // Wait for the fence to signal that command buffer has finished executing
  VK_CHECK_RESULT(
      vkWaitForFences(device, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT));

  vkDestroyFence(device, fence, nullptr);
  vkFreeCommandBuffers(device, cmdPool, 1, &commandBuffer);
}

// Build separate command buffers for every framebuffer image
// Unlike in OpenGL all rendering commands are recorded once into command
// buffers that are then resubmitted to the queue This allows to generate work
// upfront and from multiple threads, one of the biggest advantages of Vulkan
void VulkanExample::buildCommandBuffers() {
  VkCommandBufferBeginInfo cmdBufInfo = {};
  cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  cmdBufInfo.pNext = nullptr;

  // Set clear values for all framebuffer attachments with loadOp set to clear
  // We use two attachments (color and depth) that are cleared at the start of
  // the subpass and as such we need to set clear values for both
  VkClearValue clearValues[2];
  clearValues[0].color = {{0.0f, 0.0f, 0.2f, 1.0f}};
  clearValues[1].depthStencil = {1.0f, 0};

  VkRenderPassBeginInfo renderPassBeginInfo = {};
  renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassBeginInfo.pNext = nullptr;
  renderPassBeginInfo.renderPass = renderPass;
  renderPassBeginInfo.renderArea.offset.x = 0;
  renderPassBeginInfo.renderArea.offset.y = 0;
  renderPassBeginInfo.renderArea.extent.width = width;
  renderPassBeginInfo.renderArea.extent.height = height;
  renderPassBeginInfo.clearValueCount = 2;
  renderPassBeginInfo.pClearValues = clearValues;

  for (int32_t i = 0; i < drawCmdBuffers.size(); ++i) {
    // Set target frame buffer
    renderPassBeginInfo.framebuffer = frameBuffers[i];

    VK_CHECK_RESULT(vkBeginCommandBuffer(drawCmdBuffers[i], &cmdBufInfo));

    // Start the first sub pass specified in our default render pass setup by
    // the base class This will clear the color and depth attachment
    vkCmdBeginRenderPass(drawCmdBuffers[i], &renderPassBeginInfo,
                          VK_SUBPASS_CONTENTS_INLINE);

    // Update dynamic viewport state
    VkViewport viewport = {};
    viewport.height = (float)height;
    viewport.width = (float)width;
    viewport.minDepth = (float)0.0f;
    viewport.maxDepth = (float)1.0f;
    vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);

    // Update dynamic scissor state
    VkRect2D scissor = {};
    scissor.extent.width = width;
    scissor.extent.height = height;
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);

    // Bind descriptor sets describing shader binding points
    vkCmdBindDescriptorSets(drawCmdBuffers[i],
                            VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
                            0, 1, &descriptorSet, 0, nullptr);

    // Bind the rendering pipeline
    // The pipeline (state object) contains all states of the rendering
    // pipeline, binding it will set all the states specified at pipeline
    // creation time
    vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
                      pipeline);

    // Bind triangle vertex buffer (contains position and colors)
    VkDeviceSize offsets[1] = {0};
    vkCmdBindVertexBuffers(drawCmdBuffers[i], 0, 1, &vertices.buffer,
                            offsets);

    // Bind triangle index buffer
    vkCmdBindIndexBuffer(drawCmdBuffers[i], indices.buffer, 0,
                          VK_INDEX_TYPE_UINT32);

    // Draw indexed triangle
    vkCmdDrawIndexed(drawCmdBuffers[i], indices.count, 1, 0, 0, 1);

    vkCmdEndRenderPass(drawCmdBuffers[i]);

    // Ending the render pass will add an implicit barrier transitioning the
    // frame buffer color attachment to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR for
    // presenting it to the windowing system

    VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
  }
}

void VulkanExample::draw() {
  // SRS - on other platforms use original bare code with local
  // semaphores/fences for illustrative purposes Get next image in the swap
  // chain (back/front buffer)
  VkResult acquire =
      swapChain.acquireNextImage(presentCompleteSemaphore, &currentBuffer);
  if (!((acquire == VK_SUCCESS) || (acquire == VK_SUBOPTIMAL_KHR))) {
    VK_CHECK_RESULT(acquire);
  }

  // Use a fence to wait until the command buffer has finished execution
  // before using it again
  VK_CHECK_RESULT(vkWaitForFences(
      device, 1, &queueCompleteFences[currentBuffer], VK_TRUE, UINT64_MAX));
  VK_CHECK_RESULT(
      vkResetFences(device, 1, &queueCompleteFences[currentBuffer]));

  // Pipeline stage at which the queue submission will wait (via
  // pWaitSemaphores)
  VkPipelineStageFlags waitStageMask =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  // The submit info structure specifies a command buffer queue submission
  // batch
  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.pWaitDstStageMask =
      &waitStageMask;  // Pointer to the list of pipeline stages that the
                        // semaphore waits will occur at
  submitInfo.waitSemaphoreCount = 1;    // One wait semaphore
  submitInfo.signalSemaphoreCount = 1;  // One signal semaphore
  submitInfo.pCommandBuffers =
      &drawCmdBuffers[currentBuffer];  // Command buffers(s) to execute in
                                        // this batch (submission)
  submitInfo.commandBufferCount = 1;   // One command buffer

  // SRS - on other platforms use original bare code with local
  // semaphores/fences for illustrative purposes
  submitInfo.pWaitSemaphores =
      &presentCompleteSemaphore;  // Semaphore(s) to wait upon before the
                                  // submitted command buffer starts executing
  submitInfo.pSignalSemaphores =
      &renderCompleteSemaphore;  // Semaphore(s) to be signaled when command
                                  // buffers have completed

  // Submit to the graphics queue passing a wait fence
  VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo,
                                queueCompleteFences[currentBuffer]));

  // Present the current buffer to the swap chain
  // Pass the semaphore signaled by the command buffer submission from the
  // submit info as the wait semaphore for swap chain presentation This
  // ensures that the image is not presented to the windowing system until all
  // commands have been submitted
  VkResult present =
      swapChain.queuePresent(queue, currentBuffer, renderCompleteSemaphore);
  if (!((present == VK_SUCCESS) || (present == VK_SUBOPTIMAL_KHR))) {
    VK_CHECK_RESULT(present);
  }
}

// 为索引三角形准备顶点和索引缓冲区
// 还使用暂存将它们上传到设备本地内存并初始化顶点输入和属性绑定以匹配顶点着色器
void VulkanExample::prepareVertices(bool useStagingBuffers) {
  // 关于 Vulkan 内存管理的一般说明：
  // 这是一个非常复杂的主题，虽然对于一个示例应用程序来说，小的单独内存分配是可
  // 以的，但在现实世界的应用程序中，你应该一次性分配大块内存。

  // 装填顶点数据
  std::vector<Vertex> vertexBuffer = {
      // Front face
      {{-1.0f, -1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
      {{1.0f, -1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},
      {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
      {{-1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 0.0f}},

      // Back face
      {{-1.0f, -1.0f, -1.0f}, {1.0f, 0.0f, 1.0f}},
      {{1.0f, -1.0f, -1.0f}, {0.0f, 1.0f, 1.0f}},
      {{1.0f, 1.0f, -1.0f}, {1.0f, 0.0f, 1.0f}},
      {{-1.0f, 1.0f, -1.0f}, {1.0f, 1.0f, 1.0f}}};
  uint32_t vertexBufferSize =
      static_cast<uint32_t>(vertexBuffer.size()) * sizeof(Vertex);

  // 装填索引数据
  std::vector<uint32_t> indexBuffer = {// Front face
                                       0, 1, 2, 2, 3, 0,
                                       // Right face
                                       1, 5, 6, 6, 2, 1,
                                       // Back face
                                       7, 6, 5, 5, 4, 7,
                                       // Left face
                                       4, 0, 3, 3, 7, 4,
                                       // Top face
                                       3, 2, 6, 6, 7, 3,
                                       // Bottom face
                                       4, 5, 1, 1, 0, 4};
  indices.count = static_cast<uint32_t>(indexBuffer.size());
  uint32_t indexBufferSize = indices.count * sizeof(uint32_t);

  VkMemoryAllocateInfo memAlloc = {};
  memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  VkMemoryRequirements memReqs;

  void* data;

  if (useStagingBuffers) {//使用暂存缓冲区（GPU内存映射到）
    // 顶点和索引缓冲区等静态数据应存储在设备内存中，以便 GPU
    // 进行最佳（和最快）访问
    //
    // 为了实现这一点，我们使用所谓的“暂存缓冲区”：
    // - 创建一个主机可见的缓冲区（并且可以映射）
    // - 将数据复制到此缓冲区
    // - 在设备 (VRAM) 上创建另一个具有相同大小的本地缓冲区
    // - 使用命令缓冲区将数据从主机复制到设备
    // - 删除主机可见（暂存）缓冲区
    // - 使用设备本地缓冲区进行渲染

    struct StagingBuffer {
      VkDeviceMemory memory;//分配给缓冲区的内存
      VkBuffer buffer;//实际的缓冲区对象
    };

    struct {
      StagingBuffer vertices;//顶点暂存缓冲区
      StagingBuffer indices;//索引暂存缓冲区
    } stagingBuffers;

    // 顶点 buffer
    VkBufferCreateInfo vertexBufferInfo = {};
    vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vertexBufferInfo.size = vertexBufferSize;
    // 表示缓冲区将作为复制操作的源缓冲区
    vertexBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    // 创建主机可见缓冲区以将顶点数据复制到（暂存缓冲区）
    //  使用vertexBufferInfo创建一个主机可见的缓冲区，并将其句柄存储在stagingBuffers.vertices.buffer中。
    //  这个缓冲区将作为暂存缓冲区，用于将顶点数据从主机内存复制到设备内存。
    VK_CHECK_RESULT(vkCreateBuffer(device, &vertexBufferInfo, nullptr,
                                    &stagingBuffers.vertices.buffer));
    // 使用 vkGetBufferMemoryRequirements 函数查询 stagingBuffers.vertices.buffer 的内存需求，并将结果存储
    // 在 memReqs 中。设置 memAlloc.allocationSize 为 memReqs.size，表示需要分配的内存大小。
    vkGetBufferMemoryRequirements(device, stagingBuffers.vertices.buffer,
                                  &memReqs);
    memAlloc.allocationSize = memReqs.size;
    // 使用 getMemoryTypeIndex 函数为 memAlloc.memoryTypeIndex 选择一个合适的内存类型，该内存类型需具有
    // VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT和VK_MEMORY_PROPERTY_HOST_COHERENT_BIT属性。这表示内存应该是*主机
    // 可见的*，以便我们可以直接写入数据，并且*内存应该是连续的*，以便写入操作在取消映射后立即对 GPU 可见。
    memAlloc.memoryTypeIndex = getMemoryTypeIndex(
        memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    // 使用 vkAllocateMemory 为暂存缓冲区分配内存，并将其句柄存储在 stagingBuffers.vertices.memory 中
    VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr,
                                      &stagingBuffers.vertices.memory));
    // 使用 vkMapMemory 映射 stagingBuffers.vertices.memory 到主机地址空间，并将映射后的指针存储在 data 中。
    VK_CHECK_RESULT(vkMapMemory(device, stagingBuffers.vertices.memory, 0,
                                memAlloc.allocationSize, 0, &data));
    // 将顶点数据从 vertexBuffer 复制到映射的内存区域。
    memcpy(data, vertexBuffer.data(), vertexBufferSize);
    // 然后使用 vkUnmapMemory 取消映射内存。
    vkUnmapMemory(device, stagingBuffers.vertices.memory);
    // 使用 vkBindBufferMemory 将 stagingBuffers.vertices.memory 绑定到 stagingBuffers.vertices.buffer。
    VK_CHECK_RESULT(vkBindBufferMemory(device, stagingBuffers.vertices.buffer,
                                        stagingBuffers.vertices.memory, 0));

    // 创建一个设备本地缓冲区，该缓冲区将用于渲染。将 vertexBufferInfo.usage 设置为 
    // VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT，表示缓
    // 冲区将作为顶点缓冲区和复制操作的目标缓冲区使用。
    vertexBufferInfo.usage =
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    // 使用 vertexBufferInfo 创建一个设备本地缓冲区，并将其句柄存储在 vertices.buffer 中。
    VK_CHECK_RESULT(
        vkCreateBuffer(device, &vertexBufferInfo, nullptr, &vertices.buffer));
    // 使用vkGetBufferMemoryRequirements函数查询vertices.buffer的内存需求，并将结果存储在memReqs中。
    vkGetBufferMemoryRequirements(device, vertices.buffer, &memReqs);
    // 设置memAlloc.allocationSize为memReqs.size，表示需要分配的内存大小。
    memAlloc.allocationSize = memReqs.size;
    // 使用 getMemoryTypeIndex 函数为 memAlloc.memoryTypeIndex 选择一个合适的内存类型，该内存类型需具有 
    // VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT 属性。这表示内存应该是设备本地的，以便在 GPU 上高效访问。
    memAlloc.memoryTypeIndex = getMemoryTypeIndex(
        memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    // 使用 vkAllocateMemory 为设备本地缓冲区分配内存，并将其句柄存储在 vertices.memory 中。
    VK_CHECK_RESULT(
        vkAllocateMemory(device, &memAlloc, nullptr, &vertices.memory));
    // 使用 vkBindBufferMemory 将 vertices.memory 绑定到 vertices.buffer。
    VK_CHECK_RESULT(
        vkBindBufferMemory(device, vertices.buffer, vertices.memory, 0));


    // 先创建暂存缓冲区
    VkBufferCreateInfo indicesBufferInfo = {};
    indicesBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    indicesBufferInfo.size = indexBufferSize;
    indicesBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VK_CHECK_RESULT(vkCreateBuffer(device, &indicesBufferInfo, nullptr,
                                   &stagingBuffers.indices.buffer));

    // 获取缓冲区数据
    vkGetBufferMemoryRequirements(device, stagingBuffers.indices.buffer,
                                  &memReqs);
    memAlloc.allocationSize = memReqs.size;
    memAlloc.memoryTypeIndex = getMemoryTypeIndex(
        memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    // 为暂存缓冲区分配内存
    VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr,
                                     &stagingBuffers.indices.memory));
    // 将顶点数据从 vertexBuffer 复制到映射的内存区域。
    VK_CHECK_RESULT(vkMapMemory(device, stagingBuffers.indices.memory, 0,
                                memAlloc.allocationSize, 0, &data));
    memcpy(data, indexBuffer.data(), indexBufferSize);
    vkUnmapMemory(device, stagingBuffers.indices.memory);
    // 将分配的内存绑定到暂存缓冲区
    VK_CHECK_RESULT(vkBindBufferMemory(device, stagingBuffers.indices.buffer,
                                       stagingBuffers.indices.memory, 0));

    // 创建设备本地缓冲区
    indicesBufferInfo.usage =
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    VK_CHECK_RESULT(
        vkCreateBuffer(device, &indicesBufferInfo, nullptr, &indices.buffer));
    // 获取缓冲区数据
    vkGetBufferMemoryRequirements(device, stagingBuffers.indices.buffer,
                                  &memReqs);
    memAlloc.allocationSize = memReqs.size;
    memAlloc.memoryTypeIndex = getMemoryTypeIndex(
        memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    // 为本地缓冲区分配内存
    VK_CHECK_RESULT(
        vkAllocateMemory(device, &memAlloc, nullptr, &indices.memory));
    // 将分配的内存绑定到本地缓冲区
    VK_CHECK_RESULT(
        vkBindBufferMemory(device, indices.buffer, indices.memory, 0));


    VkCommandBuffer copyCmd = getCommandBuffer(true);
    VkBufferCopy copyRegion = {};

    // Vertex buffer
    copyRegion.size = vertexBufferSize;
    vkCmdCopyBuffer(copyCmd, stagingBuffers.vertices.buffer, vertices.buffer, 1,
                    &copyRegion);
    // Index buffer
    copyRegion.size = indexBufferSize;
    vkCmdCopyBuffer(copyCmd, stagingBuffers.indices.buffer, indices.buffer, 1,
                    &copyRegion);

    // Flushing the command buffer will also submit it to the queue and uses a
    // fence to ensure that all commands have been executed before returning
    flushCommandBuffer(copyCmd);

    // Destroy staging buffers
    // Note: Staging buffer must not be deleted before the copies have been
    // submitted and executed
    vkDestroyBuffer(device, stagingBuffers.vertices.buffer, nullptr);
    vkFreeMemory(device, stagingBuffers.vertices.memory, nullptr);
    vkDestroyBuffer(device, stagingBuffers.indices.buffer, nullptr);
    vkFreeMemory(device, stagingBuffers.indices.memory, nullptr);
  } else {
    // Don't use staging
    // Create host-visible buffers only and use these for rendering. This is
    // not advised and will usually result in lower rendering performance

    // Vertex buffer
    VkBufferCreateInfo vertexBufferInfo = {};
    vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vertexBufferInfo.size = vertexBufferSize;
    vertexBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

    // Copy vertex data to a buffer visible to the host
    VK_CHECK_RESULT(
        vkCreateBuffer(device, &vertexBufferInfo, nullptr, &vertices.buffer));
    vkGetBufferMemoryRequirements(device, vertices.buffer, &memReqs);
    memAlloc.allocationSize = memReqs.size;
    // VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT is host visible memory, and
    // VK_MEMORY_PROPERTY_HOST_COHERENT_BIT makes sure writes are directly
    // visible
    memAlloc.memoryTypeIndex = getMemoryTypeIndex(
        memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    VK_CHECK_RESULT(
        vkAllocateMemory(device, &memAlloc, nullptr, &vertices.memory));
    VK_CHECK_RESULT(vkMapMemory(device, vertices.memory, 0,
                                memAlloc.allocationSize, 0, &data));
    memcpy(data, vertexBuffer.data(), vertexBufferSize);
    vkUnmapMemory(device, vertices.memory);
    VK_CHECK_RESULT(
        vkBindBufferMemory(device, vertices.buffer, vertices.memory, 0));

    // Index buffer
    VkBufferCreateInfo indexbufferInfo = {};
    indexbufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    indexbufferInfo.size = indexBufferSize;
    indexbufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

    // Copy index data to a buffer visible to the host
    VK_CHECK_RESULT(
        vkCreateBuffer(device, &indexbufferInfo, nullptr, &indices.buffer));
    vkGetBufferMemoryRequirements(device, indices.buffer, &memReqs);
    memAlloc.allocationSize = memReqs.size;
    memAlloc.memoryTypeIndex = getMemoryTypeIndex(
        memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    VK_CHECK_RESULT(
        vkAllocateMemory(device, &memAlloc, nullptr, &indices.memory));
    VK_CHECK_RESULT(
        vkMapMemory(device, indices.memory, 0, indexBufferSize, 0, &data));
    memcpy(data, indexBuffer.data(), indexBufferSize);
    vkUnmapMemory(device, indices.memory);
    VK_CHECK_RESULT(
        vkBindBufferMemory(device, indices.buffer, indices.memory, 0));
  }
}

void VulkanExample::setupDescriptorPool() {
  // We need to tell the API the number of max. requested descriptors per type
  VkDescriptorPoolSize typeCounts[1];
  // This example only uses one descriptor type (uniform buffer) and only
  // requests one descriptor of this type
  typeCounts[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  typeCounts[0].descriptorCount = 1;
  // For additional types you need to add new entries in the type count list
  // E.g. for two combined image samplers :
  // typeCounts[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  // typeCounts[1].descriptorCount = 2;

  // Create the global descriptor pool
  // All descriptors used in this example are allocated from this pool
  VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
  descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  descriptorPoolInfo.pNext = nullptr;
  descriptorPoolInfo.poolSizeCount = 1;
  descriptorPoolInfo.pPoolSizes = typeCounts;
  // Set the max. number of descriptor sets that can be requested from this
  // pool (requesting beyond this limit will result in an error)
  descriptorPoolInfo.maxSets = 1;

  VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr,
                                          &descriptorPool));
}

void VulkanExample::setupDescriptorSetLayout() {
  // Setup layout of descriptors used in this example
  // Basically connects the different shader stages to descriptors for binding
  // uniform buffers, image samplers, etc. So every shader binding should map
  // to one descriptor set layout binding

  // Binding 0: Uniform buffer (Vertex shader)
  VkDescriptorSetLayoutBinding layoutBinding = {};
  layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  layoutBinding.descriptorCount = 1;
  layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  layoutBinding.pImmutableSamplers = nullptr;

  VkDescriptorSetLayoutCreateInfo descriptorLayout = {};
  descriptorLayout.sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  descriptorLayout.pNext = nullptr;
  descriptorLayout.bindingCount = 1;
  descriptorLayout.pBindings = &layoutBinding;

  VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout,
                                              nullptr, &descriptorSetLayout));

  // Create the pipeline layout that is used to generate the rendering
  // pipelines that are based on this descriptor set layout In a more complex
  // scenario you would have different pipeline layouts for different
  // descriptor set layouts that could be reused
  VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = {};
  pPipelineLayoutCreateInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pPipelineLayoutCreateInfo.pNext = nullptr;
  pPipelineLayoutCreateInfo.setLayoutCount = 1;
  pPipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;

  VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pPipelineLayoutCreateInfo,
                                          nullptr, &pipelineLayout));
}

void VulkanExample::setupDescriptorSet() {
  // Allocate a new descriptor set from the global descriptor pool
  VkDescriptorSetAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = descriptorPool;
  allocInfo.descriptorSetCount = 1;
  allocInfo.pSetLayouts = &descriptorSetLayout;

  VK_CHECK_RESULT(
      vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet));

  // Update the descriptor set determining the shader binding points
  // For every binding point used in a shader there needs to be one
  // descriptor set matching that binding point

  VkWriteDescriptorSet writeDescriptorSet = {};

  // Binding 0 : Uniform buffer
  writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeDescriptorSet.dstSet = descriptorSet;
  writeDescriptorSet.descriptorCount = 1;
  writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  writeDescriptorSet.pBufferInfo = &uniformBufferVS.descriptor;
  // Binds this uniform buffer to binding point 0
  writeDescriptorSet.dstBinding = 0;

  vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr);
}

// Create the depth (and stencil) buffer attachments used by our framebuffers
// Note: Override of virtual function in the base class and called from within
// VulkanExampleBase::prepare
void VulkanExample::setupDepthStencil() {
  // Create an optimal image used as the depth stencil attachment
  VkImageCreateInfo image = {};
  image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  image.imageType = VK_IMAGE_TYPE_2D;
  image.format = depthFormat;
  // Use example's height and width
  image.extent = {width, height, 1};
  image.mipLevels = 1;
  image.arrayLayers = 1;
  image.samples = VK_SAMPLE_COUNT_1_BIT;
  image.tiling = VK_IMAGE_TILING_OPTIMAL;
  image.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  image.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  VK_CHECK_RESULT(
      vkCreateImage(device, &image, nullptr, &depthStencil.image));

  // Allocate memory for the image (device local) and bind it to our image
  VkMemoryAllocateInfo memAlloc = {};
  memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  VkMemoryRequirements memReqs;
  vkGetImageMemoryRequirements(device, depthStencil.image, &memReqs);
  memAlloc.allocationSize = memReqs.size;
  memAlloc.memoryTypeIndex = getMemoryTypeIndex(
      memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  VK_CHECK_RESULT(
      vkAllocateMemory(device, &memAlloc, nullptr, &depthStencil.mem));
  VK_CHECK_RESULT(
      vkBindImageMemory(device, depthStencil.image, depthStencil.mem, 0));

  // Create a view for the depth stencil image
  // Images aren't directly accessed in Vulkan, but rather through views
  // described by a subresource range This allows for multiple views of one
  // image with differing ranges (e.g. for different layers)
  VkImageViewCreateInfo depthStencilView = {};
  depthStencilView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  depthStencilView.viewType = VK_IMAGE_VIEW_TYPE_2D;
  depthStencilView.format = depthFormat;
  depthStencilView.subresourceRange = {};
  depthStencilView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  // Stencil aspect should only be set on depth + stencil formats
  // (VK_FORMAT_D16_UNORM_S8_UINT..VK_FORMAT_D32_SFLOAT_S8_UINT)
  if (depthFormat >= VK_FORMAT_D16_UNORM_S8_UINT)
    depthStencilView.subresourceRange.aspectMask |=
        VK_IMAGE_ASPECT_STENCIL_BIT;

  depthStencilView.subresourceRange.baseMipLevel = 0;
  depthStencilView.subresourceRange.levelCount = 1;
  depthStencilView.subresourceRange.baseArrayLayer = 0;
  depthStencilView.subresourceRange.layerCount = 1;
  depthStencilView.image = depthStencil.image;
  VK_CHECK_RESULT(vkCreateImageView(device, &depthStencilView, nullptr,
                                    &depthStencil.view));
}

// Create a frame buffer for each swap chain image
// Note: Override of virtual function in the base class and called from within
// VulkanExampleBase::prepare
void VulkanExample::setupFrameBuffer() {
  // Create a frame buffer for every image in the swapchain
  frameBuffers.resize(swapChain.imageCount);
  for (size_t i = 0; i < frameBuffers.size(); i++) {
    std::array<VkImageView, 2> attachments;
    attachments[0] =
        swapChain.buffers[i]
            .view;  // Color attachment is the view of the swapchain image
    attachments[1] = depthStencil.view;  // Depth/Stencil attachment is the
                                          // same for all frame buffers

    VkFramebufferCreateInfo frameBufferCreateInfo = {};
    frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    // All frame buffers use the same renderpass setup
    frameBufferCreateInfo.renderPass = renderPass;
    frameBufferCreateInfo.attachmentCount =
        static_cast<uint32_t>(attachments.size());
    frameBufferCreateInfo.pAttachments = attachments.data();
    frameBufferCreateInfo.width = width;
    frameBufferCreateInfo.height = height;
    frameBufferCreateInfo.layers = 1;
    // Create the framebuffer
    VK_CHECK_RESULT(vkCreateFramebuffer(device, &frameBufferCreateInfo,
                                        nullptr, &frameBuffers[i]));
  }
}

// Render pass setup
// Render passes are a new concept in Vulkan. They describe the attachments
// used during rendering and may contain multiple subpasses with attachment
// dependencies This allows the driver to know up-front what the rendering
// will look like and is a good opportunity to optimize especially on
// tile-based renderers (with multiple subpasses) Using sub pass dependencies
// also adds implicit layout transitions for the attachment used, so we don't
// need to add explicit image memory barriers to transform them Note: Override
// of virtual function in the base class and called from within
// VulkanExampleBase::prepare
void VulkanExample::setupRenderPass() {
  // This example will use a single render pass with one subpass

  // Descriptors for the attachments used by this renderpass
  std::array<VkAttachmentDescription, 2> attachments = {};

  // Color attachment
  attachments[0].format =
      swapChain
          .colorFormat;  // Use the color format selected by the swapchain
  attachments[0].samples =
      VK_SAMPLE_COUNT_1_BIT;  // We don't use multi sampling in this example
  attachments[0].loadOp =
      VK_ATTACHMENT_LOAD_OP_CLEAR;  // Clear this attachment at the start of
                                    // the render pass
  attachments[0].storeOp =
      VK_ATTACHMENT_STORE_OP_STORE;  // Keep its contents after the render
                                      // pass is finished (for displaying it)
  attachments[0].stencilLoadOp =
      VK_ATTACHMENT_LOAD_OP_DONT_CARE;  // We don't use stencil, so don't care
                                        // for load
  attachments[0].stencilStoreOp =
      VK_ATTACHMENT_STORE_OP_DONT_CARE;  // Same for store
  attachments[0].initialLayout =
      VK_IMAGE_LAYOUT_UNDEFINED;  // Layout at render pass start. Initial
                                  // doesn't matter, so we use undefined
  attachments[0].finalLayout =
      VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;  // Layout to which the attachment is
                                        // transitioned when the render pass
                                        // is finished As we want to present
                                        // the color buffer to the swapchain,
                                        // we transition to PRESENT_KHR
  // Depth attachment
  attachments[1].format =
      depthFormat;  // A proper depth format is selected in the example base
  attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
  attachments[1].loadOp =
      VK_ATTACHMENT_LOAD_OP_CLEAR;  // Clear depth at start of first subpass
  attachments[1].storeOp =
      VK_ATTACHMENT_STORE_OP_DONT_CARE;  // We don't need depth after render
                                          // pass has finished (DONT_CARE may
                                          // result in better performance)
  attachments[1].stencilLoadOp =
      VK_ATTACHMENT_LOAD_OP_DONT_CARE;  // No stencil
  attachments[1].stencilStoreOp =
      VK_ATTACHMENT_STORE_OP_DONT_CARE;  // No Stencil
  attachments[1].initialLayout =
      VK_IMAGE_LAYOUT_UNDEFINED;  // Layout at render pass start. Initial
                                  // doesn't matter, so we use undefined
  attachments[1].finalLayout =
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;  // Transition to
                                                          // depth/stencil
                                                          // attachment

  // Setup attachment references
  VkAttachmentReference colorReference = {};
  colorReference.attachment = 0;  // Attachment 0 is color
  colorReference.layout =
      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;  // Attachment layout used as
                                                  // color during the subpass

  VkAttachmentReference depthReference = {};
  depthReference.attachment = 1;  // Attachment 1 is color
  depthReference.layout =
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;  // Attachment used as
                                                          // depth/stencil used
                                                          // during the subpass

  // Setup a single subpass reference
  VkSubpassDescription subpassDescription = {};
  subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpassDescription.colorAttachmentCount =
      1;  // Subpass uses one color attachment
  subpassDescription.pColorAttachments =
      &colorReference;  // Reference to the color attachment in slot 0
  subpassDescription.pDepthStencilAttachment =
      &depthReference;  // Reference to the depth attachment in slot 1
  subpassDescription.inputAttachmentCount =
      0;  // Input attachments can be used to sample from contents of a
          // previous subpass
  subpassDescription.pInputAttachments =
      nullptr;  // (Input attachments not used by this example)
  subpassDescription.preserveAttachmentCount =
      0;  // Preserved attachments can be used to loop (and preserve)
          // attachments through subpasses
  subpassDescription.pPreserveAttachments =
      nullptr;  // (Preserve attachments not used by this example)
  subpassDescription.pResolveAttachments =
      nullptr;  // Resolve attachments are resolved at the end of a sub pass
                // and can be used for e.g. multi sampling

  // Setup subpass dependencies
  // These will add the implicit attachment layout transitions specified by
  // the attachment descriptions The actual usage layout is preserved through
  // the layout specified in the attachment reference Each subpass dependency
  // will introduce a memory and execution dependency between the source and
  // dest subpass described by srcStageMask, dstStageMask, srcAccessMask,
  // dstAccessMask (and dependencyFlags is set) Note: VK_SUBPASS_EXTERNAL is a
  // special constant that refers to all commands executed outside of the
  // actual renderpass)
  std::array<VkSubpassDependency, 2> dependencies;

  // Does the transition from final to initial layout for the depth an color
  // attachments Depth attachment
  dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
  dependencies[0].dstSubpass = 0;
  dependencies[0].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                                  VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
  dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                                  VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
  dependencies[0].srcAccessMask =
      VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
  dependencies[0].dstAccessMask =
      VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
      VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
  dependencies[0].dependencyFlags = 0;
  // Color attachment
  dependencies[1].srcSubpass = VK_SUBPASS_EXTERNAL;
  dependencies[1].dstSubpass = 0;
  dependencies[1].srcStageMask =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[1].dstStageMask =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[1].srcAccessMask = 0;
  dependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                                  VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
  dependencies[1].dependencyFlags = 0;

  // Create the actual renderpass
  VkRenderPassCreateInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = static_cast<uint32_t>(
      attachments.size());  // Number of attachments used by this render pass
  renderPassInfo.pAttachments =
      attachments
          .data();  // Descriptions of the attachments used by the render pass
  renderPassInfo.subpassCount = 1;  // We only use one subpass in this example
  renderPassInfo.pSubpasses =
      &subpassDescription;  // Description of that subpass
  renderPassInfo.dependencyCount = static_cast<uint32_t>(
      dependencies.size());  // Number of subpass dependencies
  renderPassInfo.pDependencies =
      dependencies.data();  // Subpass dependencies used by the render pass

  VK_CHECK_RESULT(
      vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass));
}

// Vulkan loads its shaders from an immediate binary representation called
// SPIR-V Shaders are compiled offline from e.g. GLSL using the reference
// glslang compiler This function loads such a shader from a binary file and
// returns a shader module structure
VkShaderModule VulkanExample::loadSPIRVShader(std::string filename) {
  size_t shaderSize;
  char* shaderCode = NULL;

  std::ifstream is(filename, std::ios::binary | std::ios::in | std::ios::ate);

  if (is.is_open()) {
    shaderSize = is.tellg();
    is.seekg(0, std::ios::beg);
    // Copy file contents into a buffer
    shaderCode = new char[shaderSize];
    is.read(shaderCode, shaderSize);
    is.close();
    assert(shaderSize > 0);
  }
  if (shaderCode) {
    // Create a new shader module that will be used for pipeline creation
    VkShaderModuleCreateInfo moduleCreateInfo{};
    moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleCreateInfo.codeSize = shaderSize;
    moduleCreateInfo.pCode = (uint32_t*)shaderCode;

    VkShaderModule shaderModule;
    VK_CHECK_RESULT(
        vkCreateShaderModule(device, &moduleCreateInfo, NULL, &shaderModule));

    delete[] shaderCode;

    return shaderModule;
  } else {
    std::cerr << "Error: Could not open shader file \"" << filename << "\""
              << std::endl;
    return VK_NULL_HANDLE;
  }
}

stbi_uc* VulkanExample::loadTexture(std::string filename) {
  size_t shaderSize;
  char* shaderCode = NULL;

  std::ifstream is(filename, std::ios::binary | std::ios::in | std::ios::ate);

  if (is.is_open()) {
    shaderSize = is.tellg();
    is.seekg(0, std::ios::beg);
    // Copy file contents into a buffer
    shaderCode = new char[shaderSize];
    is.read(shaderCode, shaderSize);
    is.close();
    assert(shaderSize > 0);
  }
  if (shaderCode) {
    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data =
        stbi_load("./container.jpg", &width, &height, &nrChannels, 0);
    if (!data) {
      throw std::runtime_error("Failed to load texture image!");
    }
    delete[] shaderCode;
    return data;
  } else {
    std::cerr << "Error: Could not open shader file \"" << filename << "\""
              << std::endl;
    return VK_NULL_HANDLE;
  }
}

void VulkanExample::preparePipelines() {
  // Create the graphics pipeline used in this example
  // Vulkan uses the concept of rendering pipelines to encapsulate fixed
  // states, replacing OpenGL's complex state machine A pipeline is then
  // stored and hashed on the GPU making pipeline changes very fast Note:
  // There are still a few dynamic states that are not directly part of the
  // pipeline (but the info that they are used is)

  VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
  pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  // The layout used for this pipeline (can be shared among multiple pipelines
  // using the same layout)
  pipelineCreateInfo.layout = pipelineLayout;
  // Renderpass this pipeline is attached to
  pipelineCreateInfo.renderPass = renderPass;

  // Construct the different states making up the pipeline

  // Input assembly state describes how primitives are assembled
  // This pipeline will assemble vertex data as a triangle lists (though we
  // only use one triangle)
  VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {};
  inputAssemblyState.sType =
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

  // Rasterization state
  VkPipelineRasterizationStateCreateInfo rasterizationState = {};
  rasterizationState.sType =
      VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizationState.cullMode = VK_CULL_MODE_NONE;
  rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rasterizationState.depthClampEnable = VK_FALSE;
  rasterizationState.rasterizerDiscardEnable = VK_FALSE;
  rasterizationState.depthBiasEnable = VK_FALSE;
  rasterizationState.lineWidth = 1.0f;

  // Color blend state describes how blend factors are calculated (if used)
  // We need one blend attachment state per color attachment (even if blending
  // is not used)
  VkPipelineColorBlendAttachmentState blendAttachmentState[1] = {};
  blendAttachmentState[0].colorWriteMask = 0xf;
  blendAttachmentState[0].blendEnable = VK_FALSE;
  VkPipelineColorBlendStateCreateInfo colorBlendState = {};
  colorBlendState.sType =
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlendState.attachmentCount = 1;
  colorBlendState.pAttachments = blendAttachmentState;

  // Viewport state sets the number of viewports and scissor used in this
  // pipeline Note: This is actually overridden by the dynamic states (see
  // below)
  VkPipelineViewportStateCreateInfo viewportState = {};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.scissorCount = 1;

  // Enable dynamic states
  // Most states are baked into the pipeline, but there are still a few
  // dynamic states that can be changed within a command buffer To be able to
  // change these we need do specify which dynamic states will be changed
  // using this pipeline. Their actual states are set later on in the command
  // buffer. For this example we will set the viewport and scissor using
  // dynamic states
  std::vector<VkDynamicState> dynamicStateEnables;
  dynamicStateEnables.push_back(VK_DYNAMIC_STATE_VIEWPORT);
  dynamicStateEnables.push_back(VK_DYNAMIC_STATE_SCISSOR);
  VkPipelineDynamicStateCreateInfo dynamicState = {};
  dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicState.pDynamicStates = dynamicStateEnables.data();
  dynamicState.dynamicStateCount =
      static_cast<uint32_t>(dynamicStateEnables.size());

  // Depth and stencil state containing depth and stencil compare and test
  // operations We only use depth tests and want depth tests and writes to be
  // enabled and compare with less or equal
  VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
  depthStencilState.sType =
      VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depthStencilState.depthTestEnable = VK_TRUE;
  depthStencilState.depthWriteEnable = VK_TRUE;
  depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
  depthStencilState.depthBoundsTestEnable = VK_FALSE;
  depthStencilState.back.failOp = VK_STENCIL_OP_KEEP;
  depthStencilState.back.passOp = VK_STENCIL_OP_KEEP;
  depthStencilState.back.compareOp = VK_COMPARE_OP_ALWAYS;
  depthStencilState.stencilTestEnable = VK_FALSE;
  depthStencilState.front = depthStencilState.back;

  // Multi sampling state
  // This example does not make use of multi sampling (for anti-aliasing), the
  // state must still be set and passed to the pipeline
  VkPipelineMultisampleStateCreateInfo multisampleState = {};
  multisampleState.sType =
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  multisampleState.pSampleMask = nullptr;

  // Vertex input descriptions
  // Specifies the vertex input parameters for a pipeline

  // Vertex input binding
  // This example uses a single vertex input binding at binding point 0 (see
  // vkCmdBindVertexBuffers)
  VkVertexInputBindingDescription vertexInputBinding = {};
  vertexInputBinding.binding = 0;
  vertexInputBinding.stride = sizeof(Vertex);
  vertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  // Input attribute bindings describe shader attribute locations and memory
  // layouts
  std::array<VkVertexInputAttributeDescription, 2> vertexInputAttributs;
  // These match the following shader layout (see triangle.vert):
  //	layout (location = 0) in vec3 inPos;
  //	layout (location = 1) in vec3 inColor;
  // Attribute location 0: Position
  vertexInputAttributs[0].binding = 0;
  vertexInputAttributs[0].location = 0;
  // Position attribute is three 32 bit signed (SFLOAT) floats (R32 G32 B32)
  vertexInputAttributs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
  vertexInputAttributs[0].offset = offsetof(Vertex, position);
  // Attribute location 1: Color
  vertexInputAttributs[1].binding = 0;
  vertexInputAttributs[1].location = 1;
  // Color attribute is three 32 bit signed (SFLOAT) floats (R32 G32 B32)
  vertexInputAttributs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
  vertexInputAttributs[1].offset = offsetof(Vertex, color);

  // Vertex input state used for pipeline creation
  VkPipelineVertexInputStateCreateInfo vertexInputState = {};
  vertexInputState.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputState.vertexBindingDescriptionCount = 1;
  vertexInputState.pVertexBindingDescriptions = &vertexInputBinding;
  vertexInputState.vertexAttributeDescriptionCount = 2;
  vertexInputState.pVertexAttributeDescriptions = vertexInputAttributs.data();

  // Shaders
  std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};

  // Vertex shader
  shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  // Set pipeline stage for this shader
  shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  // Load binary SPIR-V shader
  shaderStages[0].module =
      loadSPIRVShader(getShadersPath() + "triangle/triangle.vert.spv");
  // Main entry point for the shader
  shaderStages[0].pName = "main";
  assert(shaderStages[0].module != VK_NULL_HANDLE);

  // Fragment shader
  shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  // Set pipeline stage for this shader
  shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  // Load binary SPIR-V shader
  shaderStages[1].module =
      loadSPIRVShader(getShadersPath() + "triangle/triangle.frag.spv");
  // Main entry point for the shader
  shaderStages[1].pName = "main";
  assert(shaderStages[1].module != VK_NULL_HANDLE);

  // Set pipeline shader stage info
  pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
  pipelineCreateInfo.pStages = shaderStages.data();

  // Assign the pipeline states to the pipeline creation info structure
  pipelineCreateInfo.pVertexInputState = &vertexInputState;
  pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
  pipelineCreateInfo.pRasterizationState = &rasterizationState;
  pipelineCreateInfo.pColorBlendState = &colorBlendState;
  pipelineCreateInfo.pMultisampleState = &multisampleState;
  pipelineCreateInfo.pViewportState = &viewportState;
  pipelineCreateInfo.pDepthStencilState = &depthStencilState;
  pipelineCreateInfo.pDynamicState = &dynamicState;

  // Create rendering pipeline using the specified states
  VK_CHECK_RESULT(vkCreateGraphicsPipelines(
      device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipeline));

  // Shader modules are no longer needed once the graphics pipeline has been
  // created
  vkDestroyShaderModule(device, shaderStages[0].module, nullptr);
  vkDestroyShaderModule(device, shaderStages[1].module, nullptr);
}

void VulkanExample::prepareUniformBuffers() {
  // 准备并初始化包含着色器制服的统一缓冲区块 Vulkan 中不再存在像 OpenGL
  // 中那样的单一制服。 所有 Shader 制服都通过统一缓冲块传递
  VkMemoryRequirements memReqs;

  // 顶点着色器统一缓冲块
  VkBufferCreateInfo bufferInfo = {};
  VkMemoryAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.pNext = nullptr;
  allocInfo.allocationSize = 0;
  allocInfo.memoryTypeIndex = 0;

  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = sizeof(uboVS);
  //该缓冲区将用作统一缓冲区
  bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

  // Create a new buffer
  VK_CHECK_RESULT(
      vkCreateBuffer(device, &bufferInfo, nullptr, &uniformBufferVS.buffer));
  // 获取内存要求，包括大小、对齐方式和内存类型
  vkGetBufferMemoryRequirements(device, uniformBufferVS.buffer, &memReqs);
  allocInfo.allocationSize = memReqs.size;
  // 获取支持主机可见内存访问的内存类型索引
  // 大多数实现提供多种内存类型，选择正确的内存类型来分配内存至关重要
  // 我们还希望缓冲区与主机一致，这样我们就不必刷新（或在每次 更新。
  // 注意：这可能会影响性能，因此您可能不希望在定期更新缓冲区的实际应用程序中执行此操作
  allocInfo.memoryTypeIndex = getMemoryTypeIndex(
      memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  // Allocate memory for the uniform buffer
  VK_CHECK_RESULT(vkAllocateMemory(device, &allocInfo, nullptr,
                                    &(uniformBufferVS.memory)));
  // Bind memory to buffer
  VK_CHECK_RESULT(vkBindBufferMemory(device, uniformBufferVS.buffer,
                                      uniformBufferVS.memory, 0));

  // 将信息存储在描述符集使用的制服描述符中
  uniformBufferVS.descriptor.buffer = uniformBufferVS.buffer;
  uniformBufferVS.descriptor.offset = 0;
  uniformBufferVS.descriptor.range = sizeof(uboVS);

  updateUniformBuffers();
}

void VulkanExample::updateUniformBuffers() {
  glm::mat4 model(1.0f);
  model = glm::rotate(model, glm::radians(45.0f), {0.0f, 1.0f, 0.0f});
  // Pass matrices to the shaders
  uboVS.projectionMatrix = camera.matrices.perspective;
  uboVS.viewMatrix = camera.matrices.view;
  uboVS.modelMatrix = model;

  // Map uniform buffer and update it
  uint8_t* pData;
  VK_CHECK_RESULT(vkMapMemory(device, uniformBufferVS.memory, 0,
                              sizeof(uboVS), 0, (void**)&pData));
  memcpy(pData, &uboVS, sizeof(uboVS));
  // Unmap after data has been copied
  // Note: Since we requested a host coherent memory type for the uniform
  // buffer, the write is instantly visible to the GPU
  vkUnmapMemory(device, uniformBufferVS.memory);
}

void VulkanExample::prepare() {
  VulkanExampleBase::prepare();		
  loadTexture();
	generateQuad();
	setupVertexDescriptions();
  prepareSynchronizationPrimitives();
  prepareVertices(USE_STAGING);
  prepareUniformBuffers();
  setupDescriptorSetLayout();
  preparePipelines();
  setupDescriptorPool();
  setupDescriptorSet();
  buildCommandBuffers();
  prepared = true;
}

void VulkanExample::render() {
  if (!prepared)
    return;
  draw();
}

void VulkanExample::viewChanged() {
  // This function is called by the base example class each time the view is
  // changed by user input
  updateUniformBuffers();
}

void VulkanExample::loadTexture() {

		// We use the Khronos texture format (https://www.khronos.org/opengles/sdk/tools/KTX/file_format_spec/)
		std::string filename = getAssetPath() + "textures/metalplate01_rgba.ktx";
		// Texture data contains 4 channels (RGBA) with unnormalized 8-bit values, this is the most commonly supported format
		VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;

		ktxResult result;
		ktxTexture* ktxTexture;

		if (!vks::tools::fileExists(filename)) {
			vks::tools::exitFatal("Could not load texture from " + filename + "\n\nThe file may be part of the additional asset pack.\n\nRun \"download_assets.py\" in the repository root to download the latest version.", -1);
		}
		result = ktxTexture_CreateFromNamedFile(filename.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTexture);

		assert(result == KTX_SUCCESS);

		// Get properties required for using and upload texture data from the ktx texture object
		texture.width = ktxTexture->baseWidth;
		texture.height = ktxTexture->baseHeight;
		texture.mipLevels = ktxTexture->numLevels;
		ktx_uint8_t *ktxTextureData = ktxTexture_GetData(ktxTexture);
		ktx_size_t ktxTextureSize = ktxTexture_GetSize(ktxTexture);

		// We prefer using staging to copy the texture data to a device local optimal image
		VkBool32 useStaging = true;

		// Only use linear tiling if forced
		bool forceLinearTiling = false;
		if (forceLinearTiling) {
			// Don't use linear if format is not supported for (linear) shader sampling
			// Get device properties for the requested texture format
			VkFormatProperties formatProperties;
			vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProperties);
			useStaging = !(formatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);
		}

		VkMemoryAllocateInfo memAllocInfo = vks::initializers::memoryAllocateInfo();
		VkMemoryRequirements memReqs = {};

		if (useStaging) {
			// Copy data to an optimal tiled image
			// This loads the texture data into a host local buffer that is copied to the optimal tiled image on the device

			// Create a host-visible staging buffer that contains the raw image data
			// This buffer will be the data source for copying texture data to the optimal tiled image on the device
			VkBuffer stagingBuffer;
			VkDeviceMemory stagingMemory;

			VkBufferCreateInfo bufferCreateInfo = vks::initializers::bufferCreateInfo();
			bufferCreateInfo.size = ktxTextureSize;
			// This buffer is used as a transfer source for the buffer copy
			bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			VK_CHECK_RESULT(vkCreateBuffer(device, &bufferCreateInfo, nullptr, &stagingBuffer));

			// Get memory requirements for the staging buffer (alignment, memory type bits)
			vkGetBufferMemoryRequirements(device, stagingBuffer, &memReqs);
			memAllocInfo.allocationSize = memReqs.size;
			// Get memory type index for a host visible buffer
			memAllocInfo.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			VK_CHECK_RESULT(vkAllocateMemory(device, &memAllocInfo, nullptr, &stagingMemory));
			VK_CHECK_RESULT(vkBindBufferMemory(device, stagingBuffer, stagingMemory, 0));

			// Copy texture data into host local staging buffer
			uint8_t *data;
			VK_CHECK_RESULT(vkMapMemory(device, stagingMemory, 0, memReqs.size, 0, (void **)&data));
			memcpy(data, ktxTextureData, ktxTextureSize);
			vkUnmapMemory(device, stagingMemory);

			// Setup buffer copy regions for each mip level
			std::vector<VkBufferImageCopy> bufferCopyRegions;
			uint32_t offset = 0;

			for (uint32_t i = 0; i < texture.mipLevels; i++) {
				// Calculate offset into staging buffer for the current mip level
				ktx_size_t offset;
				KTX_error_code ret = ktxTexture_GetImageOffset(ktxTexture, i, 0, 0, &offset);
				assert(ret == KTX_SUCCESS);
				// Setup a buffer image copy structure for the current mip level
				VkBufferImageCopy bufferCopyRegion = {};
				bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				bufferCopyRegion.imageSubresource.mipLevel = i;
				bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
				bufferCopyRegion.imageSubresource.layerCount = 1;
				bufferCopyRegion.imageExtent.width = ktxTexture->baseWidth >> i;
				bufferCopyRegion.imageExtent.height = ktxTexture->baseHeight >> i;
				bufferCopyRegion.imageExtent.depth = 1;
				bufferCopyRegion.bufferOffset = offset;
				bufferCopyRegions.push_back(bufferCopyRegion);
			}

			// Create optimal tiled target image on the device
			VkImageCreateInfo imageCreateInfo = vks::initializers::imageCreateInfo();
			imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
			imageCreateInfo.format = format;
			imageCreateInfo.mipLevels = texture.mipLevels;
			imageCreateInfo.arrayLayers = 1;
			imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			// Set initial layout of the image to undefined
			imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageCreateInfo.extent = { texture.width, texture.height, 1 };
			imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			VK_CHECK_RESULT(vkCreateImage(device, &imageCreateInfo, nullptr, &texture.image));

			vkGetImageMemoryRequirements(device, texture.image, &memReqs);
			memAllocInfo.allocationSize = memReqs.size;
			memAllocInfo.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			VK_CHECK_RESULT(vkAllocateMemory(device, &memAllocInfo, nullptr, &texture.deviceMemory));
			VK_CHECK_RESULT(vkBindImageMemory(device, texture.image, texture.deviceMemory, 0));

			VkCommandBuffer copyCmd = vulkanDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

			// Image memory barriers for the texture image

			// The sub resource range describes the regions of the image that will be transitioned using the memory barriers below
			VkImageSubresourceRange subresourceRange = {};
			// Image only contains color data
			subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			// Start at first mip level
			subresourceRange.baseMipLevel = 0;
			// We will transition on all mip levels
			subresourceRange.levelCount = texture.mipLevels;
			// The 2D texture only has one layer
			subresourceRange.layerCount = 1;

			// Transition the texture image layout to transfer target, so we can safely copy our buffer data to it.
			VkImageMemoryBarrier imageMemoryBarrier = vks::initializers::imageMemoryBarrier();;
			imageMemoryBarrier.image = texture.image;
			imageMemoryBarrier.subresourceRange = subresourceRange;
			imageMemoryBarrier.srcAccessMask = 0;
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

			// Insert a memory dependency at the proper pipeline stages that will execute the image layout transition
			// Source pipeline stage is host write/read execution (VK_PIPELINE_STAGE_HOST_BIT)
			// Destination pipeline stage is copy command execution (VK_PIPELINE_STAGE_TRANSFER_BIT)
			vkCmdPipelineBarrier(
				copyCmd,
				VK_PIPELINE_STAGE_HOST_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &imageMemoryBarrier);

			// Copy mip levels from staging buffer
			vkCmdCopyBufferToImage(
				copyCmd,
				stagingBuffer,
				texture.image,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				static_cast<uint32_t>(bufferCopyRegions.size()),
				bufferCopyRegions.data());

			// Once the data has been uploaded we transfer to the texture image to the shader read layout, so it can be sampled from
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			// Insert a memory dependency at the proper pipeline stages that will execute the image layout transition
			// Source pipeline stage is copy command execution (VK_PIPELINE_STAGE_TRANSFER_BIT)
			// Destination pipeline stage fragment shader access (VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT)
			vkCmdPipelineBarrier(
				copyCmd,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &imageMemoryBarrier);

			// Store current layout for later reuse
			texture.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			vulkanDevice->flushCommandBuffer(copyCmd, queue, true);

			// Clean up staging resources
			vkFreeMemory(device, stagingMemory, nullptr);
			vkDestroyBuffer(device, stagingBuffer, nullptr);
		} else {
			// Copy data to a linear tiled image

			VkImage mappableImage;
			VkDeviceMemory mappableMemory;

			// Load mip map level 0 to linear tiling image
			VkImageCreateInfo imageCreateInfo = vks::initializers::imageCreateInfo();
			imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
			imageCreateInfo.format = format;
			imageCreateInfo.mipLevels = 1;
			imageCreateInfo.arrayLayers = 1;
			imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			imageCreateInfo.tiling = VK_IMAGE_TILING_LINEAR;
			imageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
			imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
			imageCreateInfo.extent = { texture.width, texture.height, 1 };
			VK_CHECK_RESULT(vkCreateImage(device, &imageCreateInfo, nullptr, &mappableImage));

			// Get memory requirements for this image like size and alignment
			vkGetImageMemoryRequirements(device, mappableImage, &memReqs);
			// Set memory allocation size to required memory size
			memAllocInfo.allocationSize = memReqs.size;
			// Get memory type that can be mapped to host memory
			memAllocInfo.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			VK_CHECK_RESULT(vkAllocateMemory(device, &memAllocInfo, nullptr, &mappableMemory));
			VK_CHECK_RESULT(vkBindImageMemory(device, mappableImage, mappableMemory, 0));

			// Map image memory
			void *data;
			VK_CHECK_RESULT(vkMapMemory(device, mappableMemory, 0, memReqs.size, 0, &data));
			// Copy image data of the first mip level into memory
			memcpy(data, ktxTextureData, memReqs.size);
			vkUnmapMemory(device, mappableMemory);

			// Linear tiled images don't need to be staged and can be directly used as textures
			texture.image = mappableImage;
			texture.deviceMemory = mappableMemory;
			texture.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			// Setup image memory barrier transfer image to shader read layout
			VkCommandBuffer copyCmd = vulkanDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

			// The sub resource range describes the regions of the image we will be transition
			VkImageSubresourceRange subresourceRange = {};
			subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			subresourceRange.baseMipLevel = 0;
			subresourceRange.levelCount = 1;
			subresourceRange.layerCount = 1;

			// Transition the texture image layout to shader read, so it can be sampled from
			VkImageMemoryBarrier imageMemoryBarrier = vks::initializers::imageMemoryBarrier();;
			imageMemoryBarrier.image = texture.image;
			imageMemoryBarrier.subresourceRange = subresourceRange;
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
			imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			// Insert a memory dependency at the proper pipeline stages that will execute the image layout transition
			// Source pipeline stage is host write/read execution (VK_PIPELINE_STAGE_HOST_BIT)
			// Destination pipeline stage fragment shader access (VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT)
			vkCmdPipelineBarrier(
				copyCmd,
				VK_PIPELINE_STAGE_HOST_BIT,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &imageMemoryBarrier);

			vulkanDevice->flushCommandBuffer(copyCmd, queue, true);
		}

		ktxTexture_Destroy(ktxTexture);

		// Create a texture sampler
		// In Vulkan textures are accessed by samplers
		// This separates all the sampling information from the texture data. This means you could have multiple sampler objects for the same texture with different settings
		// Note: Similar to the samplers available with OpenGL 3.3
		VkSamplerCreateInfo sampler = vks::initializers::samplerCreateInfo();
		sampler.magFilter = VK_FILTER_LINEAR;
		sampler.minFilter = VK_FILTER_LINEAR;
		sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		sampler.mipLodBias = 0.0f;
		sampler.compareOp = VK_COMPARE_OP_NEVER;
		sampler.minLod = 0.0f;
		// Set max level-of-detail to mip level count of the texture
		sampler.maxLod = (useStaging) ? (float)texture.mipLevels : 0.0f;
		// Enable anisotropic filtering
		// This feature is optional, so we must check if it's supported on the device
		if (vulkanDevice->features.samplerAnisotropy) {
			// Use max. level of anisotropy for this example
			sampler.maxAnisotropy = vulkanDevice->properties.limits.maxSamplerAnisotropy;
			sampler.anisotropyEnable = VK_TRUE;
		} else {
			// The device does not support anisotropic filtering
			sampler.maxAnisotropy = 1.0;
			sampler.anisotropyEnable = VK_FALSE;
		}
		sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		VK_CHECK_RESULT(vkCreateSampler(device, &sampler, nullptr, &texture.sampler));

		// Create image view
		// Textures are not directly accessed by the shaders and
		// are abstracted by image views containing additional
		// information and sub resource ranges
		VkImageViewCreateInfo view = vks::initializers::imageViewCreateInfo();
		view.viewType = VK_IMAGE_VIEW_TYPE_2D;
		view.format = format;
		// The subresource range describes the set of mip levels (and array layers) that can be accessed through this image view
		// It's possible to create multiple image views for a single image referring to different (and/or overlapping) ranges of the image
		view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		view.subresourceRange.baseMipLevel = 0;
		view.subresourceRange.baseArrayLayer = 0;
		view.subresourceRange.layerCount = 1;
		// Linear tiling usually won't support mip maps
		// Only set mip map count if optimal tiling is used
		view.subresourceRange.levelCount = (useStaging) ? texture.mipLevels : 1;
		// The view will be based on the texture's image
		view.image = texture.image;
		VK_CHECK_RESULT(vkCreateImageView(device, &view, nullptr, &texture.view));
}