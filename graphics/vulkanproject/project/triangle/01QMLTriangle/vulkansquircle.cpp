// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "vulkansquircle.h"
#include <QtCore/QRunnable>
#include <QtQuick/QQuickWindow>

#include <QVulkanInstance>
#include <QVulkanFunctions>
#include <QFile>
#include <windows.h>
#include <set>
#include <string>
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>
#include <istream>
#include <iostream>
#include <fstream>

//SquircleRenderer 类：
//    SquircleRenderer 类主要负责实现用于渲染squircle形状的Vulkan渲染过程。
// 它负责初始化Vulkan资源（如缓冲区、管线、描述符集等）、创建图形管线以及
// 执行渲染命令。主要功能如下：
//    初始化Vulkan实例、设备、物理设备等资源； 创建顶点和uniform缓冲区；
//    创建图形管线，包括着色器模块、顶点输入、视口状态、光栅化状态、混合状态等；
//    创建描述符集和描述符池；
//    创建渲染命令，包括记录渲染命令到命令缓冲区；
//    渲染循环，每帧更新uniform缓冲区的数据并执行渲染命令。 

class SquircleRenderer : public QObject
{
    Q_OBJECT
public:
    ~SquircleRenderer();

    void setT(qreal t) { m_t = t; }
    void setViewportSize(const QSize &size) { m_viewportSize = size; }
    void setWindow(QQuickWindow *window) { m_window = window; }

public slots:
    void frameStart();
    void mainPassRecordingStart();

private:
    enum Stage {
        VertexStage,
        FragmentStage
    };
    void prepareShader(Stage stage);
    void init(int framesInFlight);

    QSize m_viewportSize;
    qreal m_t = 0;
    QQuickWindow *m_window;

    QByteArray m_vert;
    QByteArray m_frag;

    bool m_initialized = false;
    VkPhysicalDevice m_physDev = VK_NULL_HANDLE;
    VkDevice m_dev = VK_NULL_HANDLE;
    QVulkanDeviceFunctions *m_devFuncs = nullptr;
    QVulkanFunctions *m_funcs = nullptr;

    VkBuffer m_vbuf = VK_NULL_HANDLE;
    VkDeviceMemory m_vbufMem = VK_NULL_HANDLE;
    VkBuffer m_ubuf = VK_NULL_HANDLE;
    VkDeviceMemory m_ubufMem = VK_NULL_HANDLE;
    VkDeviceSize m_allocPerUbuf = 0;

    VkPipelineCache m_pipelineCache = VK_NULL_HANDLE;

    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_resLayout = VK_NULL_HANDLE;
    VkPipeline m_pipeline = VK_NULL_HANDLE;

    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet m_ubufDescriptor = VK_NULL_HANDLE;
};

VulkanSquircle::VulkanSquircle()
{
    connect(this, &QQuickItem::windowChanged, this, &VulkanSquircle::handleWindowChanged);
}

void VulkanSquircle::setT(qreal t)
{
    if (t == m_t)
        return;
    m_t = t;
    emit tChanged();
    if (window())
        window()->update();
}

void VulkanSquircle::handleWindowChanged(QQuickWindow *win)
{
    if (win) {
        connect(win, &QQuickWindow::beforeSynchronizing, this, &VulkanSquircle::sync, Qt::DirectConnection);
        connect(win, &QQuickWindow::sceneGraphInvalidated, this, &VulkanSquircle::cleanup, Qt::DirectConnection);

        // Ensure we start with cleared to black. The squircle's blend mode relies on this.
        win->setColor(Qt::black);
    }
}

// The safe way to release custom graphics resources is to both connect to
// sceneGraphInvalidated() and implement releaseResources(). To support
// threaded render loops the latter performs the SquircleRenderer destruction
// via scheduleRenderJob(). Note that the VulkanSquircle may be gone by the time
// the QRunnable is invoked.

void VulkanSquircle::cleanup()
{
    delete m_renderer;
    m_renderer = nullptr;
}

class CleanupJob : public QRunnable
{
public:
    CleanupJob(SquircleRenderer *renderer) : m_renderer(renderer) { }
    void run() override { delete m_renderer; }
private:
    SquircleRenderer *m_renderer;
};

void VulkanSquircle::releaseResources()
{
    window()->scheduleRenderJob(new CleanupJob(m_renderer), QQuickWindow::BeforeSynchronizingStage);
    m_renderer = nullptr;
}

SquircleRenderer::~SquircleRenderer()
{
    qDebug("cleanup");
    if (!m_devFuncs)
        return;

    m_devFuncs->vkDestroyPipeline(m_dev, m_pipeline, nullptr);
    m_devFuncs->vkDestroyPipelineLayout(m_dev, m_pipelineLayout, nullptr);
    m_devFuncs->vkDestroyDescriptorSetLayout(m_dev, m_resLayout, nullptr);

    m_devFuncs->vkDestroyDescriptorPool(m_dev, m_descriptorPool, nullptr);

    m_devFuncs->vkDestroyPipelineCache(m_dev, m_pipelineCache, nullptr);

    m_devFuncs->vkDestroyBuffer(m_dev, m_vbuf, nullptr);
    m_devFuncs->vkFreeMemory(m_dev, m_vbufMem, nullptr);

    m_devFuncs->vkDestroyBuffer(m_dev, m_ubuf, nullptr);
    m_devFuncs->vkFreeMemory(m_dev, m_ubufMem, nullptr);

    qDebug("released");
}

//   方法在*每帧*之前调用，以确保 SquircleRenderer 与当前状态同步。
// 例如，它将窗口大小、动画进度等传递给渲染器
void VulkanSquircle::sync()
{
    if (!m_renderer) {
        m_renderer = new SquircleRenderer;
        // Initializing resources is done before starting to record the
        // renderpass, regardless of wanting an underlay or overlay.
        connect(window(), &QQuickWindow::beforeRendering, m_renderer, &SquircleRenderer::frameStart, Qt::DirectConnection);
        // Here we want an underlay and therefore connect to
        // beforeRenderPassRecording. Changing to afterRenderPassRecording
        // would render the squircle on top (overlay).
        connect(window(), &QQuickWindow::beforeRenderPassRecording, m_renderer, &SquircleRenderer::mainPassRecordingStart, Qt::DirectConnection);
    }
    m_renderer->setViewportSize(window()->size() * window()->devicePixelRatio());
    //m_renderer->setT(m_t);
    m_renderer->setWindow(window());
}

//方法在*每帧*的开始时调用。它确保Vulkan设备已经初始化，准备好进行渲染。
void SquircleRenderer::frameStart()
{
    QSGRendererInterface *rif = m_window->rendererInterface();

    // We are not prepared for anything other than running with the RHI and its Vulkan backend.
    Q_ASSERT(rif->graphicsApi() == QSGRendererInterface::Vulkan);

    if (m_vert.isEmpty())
        prepareShader(VertexStage);
    if (m_frag.isEmpty())
        prepareShader(FragmentStage);

    if (!m_initialized)
        init(m_window->graphicsStateInfo().framesInFlight);
}

static const float vertices[] = {
    -1, -1,
    1, -1,
    -1, 1,
    1, 1
};

const int UBUF_SIZE = 4;

//    方法在Qt Quick场景的主渲染通道开始时调用。
//    在这个方法中，Vulkan 渲染命令被记录
// 到当前帧的命令缓冲区中，用于绘制曲线矩形。
void SquircleRenderer::mainPassRecordingStart() {  
  // 此示例演示了简单的情况：将一些命令添加到场景图的主渲染通道。
  // 它不会创建自己的通道、渲染目标等，因此不需要同步。

    const QQuickWindow::GraphicsStateInfo &stateInfo(m_window->graphicsStateInfo());
    QSGRendererInterface *rif = m_window->rendererInterface();

    VkDeviceSize ubufOffset = stateInfo.currentFrameSlot * m_allocPerUbuf;
    void *p = nullptr;
    VkResult err = m_devFuncs->vkMapMemory(m_dev, m_ubufMem, ubufOffset, m_allocPerUbuf, 0, &p);
    if (err != VK_SUCCESS || !p)
        qFatal("Failed to map uniform buffer memory: %d", err);
    float t = m_t;
    memcpy(p, &t, 4);
    m_devFuncs->vkUnmapMemory(m_dev, m_ubufMem);

    m_window->beginExternalCommands();

    // Must query the command buffer _after_ beginExternalCommands(), this is
    // actually important when running on Vulkan because what we get here is a
    // new secondary command buffer, not the primary one.
    VkCommandBuffer cb = *reinterpret_cast<VkCommandBuffer *>(
                rif->getResource(m_window, QSGRendererInterface::CommandListResource));
    Q_ASSERT(cb);

    // Do not assume any state persists on the command buffer. (it may be a
    // brand new one that just started recording)

    m_devFuncs->vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

    VkDeviceSize vbufOffset = 0;
    m_devFuncs->vkCmdBindVertexBuffers(cb, 0, 1, &m_vbuf, &vbufOffset);

    uint32_t dynamicOffset = m_allocPerUbuf * stateInfo.currentFrameSlot;
    m_devFuncs->vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1,
                                        &m_ubufDescriptor, 1, &dynamicOffset);

    VkViewport vp = { 0, 0, float(m_viewportSize.width()), float(m_viewportSize.height()), 0.0f, 1.0f };
    m_devFuncs->vkCmdSetViewport(cb, 0, 1, &vp);
    VkRect2D scissor = { { 0, 0 }, { uint32_t(m_viewportSize.width()), uint32_t(m_viewportSize.height()) } };
    m_devFuncs->vkCmdSetScissor(cb, 0, 1, &scissor);

    m_devFuncs->vkCmdDraw(cb, 4, 1, 0, 0);

    m_window->endExternalCommands();
}

//方法从文件中加载顶点和片段着色器，这些着色器将用于渲染曲线矩形。
void SquircleRenderer::prepareShader(Stage stage)
{
    QString filename;
    if (stage == VertexStage) {
        filename = QLatin1String("./squircle.vert.spv");
    } else {
        Q_ASSERT(stage == FragmentStage);
        filename = QLatin1String("./squircle.frag.spv");
    }
    QFile f(filename);
    if (!f.open(QIODevice::ReadOnly))
        qFatal("Failed to read shader %s", qPrintable(filename));

    const QByteArray contents = f.readAll();

    if (stage == VertexStage) {
        m_vert = contents;
        Q_ASSERT(!m_vert.isEmpty());
    } else {
        m_frag = contents;
        Q_ASSERT(!m_frag.isEmpty());
    }
}

static inline VkDeviceSize aligned(VkDeviceSize v, VkDeviceSize byteAlign)
{
    return (v + byteAlign - 1) & ~(byteAlign - 1);
}

//始化SquircleRenderer类的Vulkan相关资源
void SquircleRenderer::init(int framesInFlight)
{
    qDebug("init");

    Q_ASSERT(framesInFlight <= 3);
    m_initialized = true;

    QSGRendererInterface *rif = m_window->rendererInterface();
    QVulkanInstance *inst = reinterpret_cast<QVulkanInstance *>(
                rif->getResource(m_window, QSGRendererInterface::VulkanInstanceResource));
    Q_ASSERT(inst && inst->isValid());

    m_physDev = *reinterpret_cast<VkPhysicalDevice *>(rif->getResource(m_window, QSGRendererInterface::PhysicalDeviceResource));
    m_dev = *reinterpret_cast<VkDevice *>(rif->getResource(m_window, QSGRendererInterface::DeviceResource));
    Q_ASSERT(m_physDev && m_dev);

    m_devFuncs = inst->deviceFunctions(m_dev);
    m_funcs = inst->functions();
    Q_ASSERT(m_devFuncs && m_funcs);

    VkRenderPass rp = *reinterpret_cast<VkRenderPass *>(
                rif->getResource(m_window, QSGRendererInterface::RenderPassResource));
    Q_ASSERT(rp);

    // For simplicity we just use host visible buffers instead of device local + staging.

    VkPhysicalDeviceProperties physDevProps;
    m_funcs->vkGetPhysicalDeviceProperties(m_physDev, &physDevProps);

    VkPhysicalDeviceMemoryProperties physDevMemProps;
    m_funcs->vkGetPhysicalDeviceMemoryProperties(m_physDev, &physDevMemProps);

    VkBufferCreateInfo bufferInfo;
    memset(&bufferInfo, 0, sizeof(bufferInfo));
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = sizeof(vertices);
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    VkResult err = m_devFuncs->vkCreateBuffer(m_dev, &bufferInfo, nullptr, &m_vbuf);
    if (err != VK_SUCCESS)
        qFatal("Failed to create vertex buffer: %d", err);

    VkMemoryRequirements memReq;
    m_devFuncs->vkGetBufferMemoryRequirements(m_dev, m_vbuf, &memReq);
    VkMemoryAllocateInfo allocInfo;
    memset(&allocInfo, 0, sizeof(allocInfo));
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReq.size;

    uint32_t memTypeIndex = uint32_t(-1);
    const VkMemoryType *memType = physDevMemProps.memoryTypes;
    for (uint32_t i = 0; i < physDevMemProps.memoryTypeCount; ++i) {
        if (memReq.memoryTypeBits & (1 << i)) {
            if ((memType[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
                    && (memType[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
            {
                    memTypeIndex = i;
                    break;
            }
        }
    }
    if (memTypeIndex == uint32_t(-1))
        qFatal("Failed to find host visible and coherent memory type");

    allocInfo.memoryTypeIndex = memTypeIndex;
    err = m_devFuncs->vkAllocateMemory(m_dev, &allocInfo, nullptr, &m_vbufMem);
    if (err != VK_SUCCESS)
        qFatal("Failed to allocate vertex buffer memory of size %u: %d", uint(allocInfo.allocationSize), err);

    void *p = nullptr;
    err = m_devFuncs->vkMapMemory(m_dev, m_vbufMem, 0, allocInfo.allocationSize, 0, &p);
    if (err != VK_SUCCESS || !p)
        qFatal("Failed to map vertex buffer memory: %d", err);
    memcpy(p, vertices, sizeof(vertices));
    m_devFuncs->vkUnmapMemory(m_dev, m_vbufMem);
    err = m_devFuncs->vkBindBufferMemory(m_dev, m_vbuf, m_vbufMem, 0);
    if (err != VK_SUCCESS)
        qFatal("Failed to bind vertex buffer memory: %d", err);

    // Now have a uniform buffer with enough space for the buffer data for each
    // (potentially) in-flight frame. (as we will write the contents every
    // frame, and so would need to wait for command buffer completion if there
    // was only one, and that would not be nice)

    // Could have three buffers and three descriptor sets, or one buffer and
    // one descriptor set and dynamic offset. We chose the latter in this
    // example.

    // We use one memory allocation for all uniform buffers, but then have to
    // watch out for the buffer offset alignment requirement, which may be as
    // large as 256 bytes.

    m_allocPerUbuf = aligned(UBUF_SIZE, physDevProps.limits.minUniformBufferOffsetAlignment);

    bufferInfo.size = framesInFlight * m_allocPerUbuf;
    bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    err = m_devFuncs->vkCreateBuffer(m_dev, &bufferInfo, nullptr, &m_ubuf);
    if (err != VK_SUCCESS)
        qFatal("Failed to create uniform buffer: %d", err);
    m_devFuncs->vkGetBufferMemoryRequirements(m_dev, m_ubuf, &memReq);
    memTypeIndex = -1;
    for (uint32_t i = 0; i < physDevMemProps.memoryTypeCount; ++i) {
        if (memReq.memoryTypeBits & (1 << i)) {
            if ((memType[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
                    && (memType[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
            {
                    memTypeIndex = i;
                    break;
            }
        }
    }
    if (memTypeIndex == uint32_t(-1))
        qFatal("Failed to find host visible and coherent memory type");

    allocInfo.allocationSize = framesInFlight * m_allocPerUbuf;
    allocInfo.memoryTypeIndex = memTypeIndex;
    err = m_devFuncs->vkAllocateMemory(m_dev, &allocInfo, nullptr, &m_ubufMem);
    if (err != VK_SUCCESS)
        qFatal("Failed to allocate uniform buffer memory of size %u: %d", uint(allocInfo.allocationSize), err);

    err = m_devFuncs->vkBindBufferMemory(m_dev, m_ubuf, m_ubufMem, 0);
    if (err != VK_SUCCESS)
        qFatal("Failed to bind uniform buffer memory: %d", err);

    // Now onto the pipeline.

    VkPipelineCacheCreateInfo pipelineCacheInfo;
    memset(&pipelineCacheInfo, 0, sizeof(pipelineCacheInfo));
    pipelineCacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    err = m_devFuncs->vkCreatePipelineCache(m_dev, &pipelineCacheInfo, nullptr, &m_pipelineCache);
    if (err != VK_SUCCESS)
        qFatal("Failed to create pipeline cache: %d", err);

    VkDescriptorSetLayoutBinding descLayoutBinding;
    memset(&descLayoutBinding, 0, sizeof(descLayoutBinding));
    descLayoutBinding.binding = 0;
    descLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    descLayoutBinding.descriptorCount = 1;
    descLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    VkDescriptorSetLayoutCreateInfo layoutInfo;
    memset(&layoutInfo, 0, sizeof(layoutInfo));
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &descLayoutBinding;
    err = m_devFuncs->vkCreateDescriptorSetLayout(m_dev, &layoutInfo, nullptr, &m_resLayout);
    if (err != VK_SUCCESS)
        qFatal("Failed to create descriptor set layout: %d", err);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo;
    memset(&pipelineLayoutInfo, 0, sizeof(pipelineLayoutInfo));
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_resLayout;
    err = m_devFuncs->vkCreatePipelineLayout(m_dev, &pipelineLayoutInfo, nullptr, &m_pipelineLayout);
    if (err != VK_SUCCESS)
        qWarning("Failed to create pipeline layout: %d", err);

    VkGraphicsPipelineCreateInfo pipelineInfo;
    memset(&pipelineInfo, 0, sizeof(pipelineInfo));
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

    VkShaderModuleCreateInfo shaderInfo;
    memset(&shaderInfo, 0, sizeof(shaderInfo));
    shaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderInfo.codeSize = m_vert.size();
    shaderInfo.pCode = reinterpret_cast<const quint32 *>(m_vert.constData());
    VkShaderModule vertShaderModule;
    err = m_devFuncs->vkCreateShaderModule(m_dev, &shaderInfo, nullptr, &vertShaderModule);
    if (err != VK_SUCCESS)
        qFatal("Failed to create vertex shader module: %d", err);

    shaderInfo.codeSize = m_frag.size();
    shaderInfo.pCode = reinterpret_cast<const quint32 *>(m_frag.constData());
    VkShaderModule fragShaderModule;
    err = m_devFuncs->vkCreateShaderModule(m_dev, &shaderInfo, nullptr, &fragShaderModule);
    if (err != VK_SUCCESS)
        qFatal("Failed to create fragment shader module: %d", err);

    VkPipelineShaderStageCreateInfo stageInfo[2];
    memset(&stageInfo, 0, sizeof(stageInfo));
    stageInfo[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageInfo[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stageInfo[0].module = vertShaderModule;
    stageInfo[0].pName = "main";
    stageInfo[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageInfo[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stageInfo[1].module = fragShaderModule;
    stageInfo[1].pName = "main";
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = stageInfo;

    VkVertexInputBindingDescription vertexBinding = {
        0, // binding
        2 * sizeof(float), // stride
        VK_VERTEX_INPUT_RATE_VERTEX
    };
    VkVertexInputAttributeDescription vertexAttr = {
        0, // location
        0, // binding
        VK_FORMAT_R32G32_SFLOAT, // 'vertices' only has 2 floats per vertex
        0 // offset
    };
    VkPipelineVertexInputStateCreateInfo vertexInputInfo;
    memset(&vertexInputInfo, 0, sizeof(vertexInputInfo));
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &vertexBinding;
    vertexInputInfo.vertexAttributeDescriptionCount = 1;
    vertexInputInfo.pVertexAttributeDescriptions = &vertexAttr;
    pipelineInfo.pVertexInputState = &vertexInputInfo;

    VkDynamicState dynStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicInfo;
    memset(&dynamicInfo, 0, sizeof(dynamicInfo));
    dynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicInfo.dynamicStateCount = 2;
    dynamicInfo.pDynamicStates = dynStates;
    pipelineInfo.pDynamicState = &dynamicInfo;

    VkPipelineViewportStateCreateInfo viewportInfo;
    memset(&viewportInfo, 0, sizeof(viewportInfo));
    viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportInfo.viewportCount = viewportInfo.scissorCount = 1;
    pipelineInfo.pViewportState = &viewportInfo;

    VkPipelineInputAssemblyStateCreateInfo iaInfo;
    memset(&iaInfo, 0, sizeof(iaInfo));
    iaInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    iaInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    pipelineInfo.pInputAssemblyState = &iaInfo;

    VkPipelineRasterizationStateCreateInfo rsInfo;
    memset(&rsInfo, 0, sizeof(rsInfo));
    rsInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rsInfo.lineWidth = 1.0f;
    pipelineInfo.pRasterizationState = &rsInfo;

    VkPipelineMultisampleStateCreateInfo msInfo;
    memset(&msInfo, 0, sizeof(msInfo));
    msInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    msInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    pipelineInfo.pMultisampleState = &msInfo;

    VkPipelineDepthStencilStateCreateInfo dsInfo;
    memset(&dsInfo, 0, sizeof(dsInfo));
    dsInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    pipelineInfo.pDepthStencilState = &dsInfo;

    // SrcAlpha, One
    VkPipelineColorBlendStateCreateInfo blendInfo;
    memset(&blendInfo, 0, sizeof(blendInfo));
    blendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    VkPipelineColorBlendAttachmentState blend;
    memset(&blend, 0, sizeof(blend));
    blend.blendEnable = true;
    blend.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blend.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
    blend.colorBlendOp = VK_BLEND_OP_ADD;
    blend.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    blend.alphaBlendOp = VK_BLEND_OP_ADD;
    blend.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT
            | VK_COLOR_COMPONENT_A_BIT;
    blendInfo.attachmentCount = 1;
    blendInfo.pAttachments = &blend;
    pipelineInfo.pColorBlendState = &blendInfo;

    pipelineInfo.layout = m_pipelineLayout;

    pipelineInfo.renderPass = rp;

    err = m_devFuncs->vkCreateGraphicsPipelines(m_dev, m_pipelineCache, 1, &pipelineInfo, nullptr, &m_pipeline);

    m_devFuncs->vkDestroyShaderModule(m_dev, vertShaderModule, nullptr);
    m_devFuncs->vkDestroyShaderModule(m_dev, fragShaderModule, nullptr);

    if (err != VK_SUCCESS)
        qFatal("Failed to create graphics pipeline: %d", err);

    // Now just need some descriptors.
    VkDescriptorPoolSize descPoolSizes[] = {
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1 }
    };
    VkDescriptorPoolCreateInfo descPoolInfo;
    memset(&descPoolInfo, 0, sizeof(descPoolInfo));
    descPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descPoolInfo.flags = 0; // won't use vkFreeDescriptorSets
    descPoolInfo.maxSets = 1;
    descPoolInfo.poolSizeCount = sizeof(descPoolSizes) / sizeof(descPoolSizes[0]);
    descPoolInfo.pPoolSizes = descPoolSizes;
    err = m_devFuncs->vkCreateDescriptorPool(m_dev, &descPoolInfo, nullptr, &m_descriptorPool);
    if (err != VK_SUCCESS)
        qFatal("Failed to create descriptor pool: %d", err);

    VkDescriptorSetAllocateInfo descAllocInfo;
    memset(&descAllocInfo, 0, sizeof(descAllocInfo));
    descAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descAllocInfo.descriptorPool = m_descriptorPool;
    descAllocInfo.descriptorSetCount = 1;
    descAllocInfo.pSetLayouts = &m_resLayout;
    err = m_devFuncs->vkAllocateDescriptorSets(m_dev, &descAllocInfo, &m_ubufDescriptor);
    if (err != VK_SUCCESS)
        qFatal("Failed to allocate descriptor set");

    VkWriteDescriptorSet writeInfo;
    memset(&writeInfo, 0, sizeof(writeInfo));
    writeInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeInfo.dstSet = m_ubufDescriptor;
    writeInfo.dstBinding = 0;
    writeInfo.descriptorCount = 1;
    writeInfo.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    VkDescriptorBufferInfo bufInfo;
    bufInfo.buffer = m_ubuf;
    bufInfo.offset = 0; // dynamic offset is used so this is ignored
    bufInfo.range = UBUF_SIZE;
    writeInfo.pBufferInfo = &bufInfo;
    m_devFuncs->vkUpdateDescriptorSets(m_dev, 1, &writeInfo, 0, nullptr);
}




const int WIDTH = 800;
const int HEIGHT = 600;

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

const bool enableValidationLayers = true;

VulkanTriangle::VulkanTriangle() {
  connect(this, &QQuickItem::windowChanged, this,
          &VulkanTriangle::handleWindowChanged);
}

void VulkanTriangle::handleWindowChanged(QQuickWindow* win) {
  if (win) {
     //run();
    connect(window(), &QQuickWindow::beforeRendering, this,
            &VulkanTriangle::run, Qt::QueuedConnection);
    connect(window(), &QQuickWindow::beforeRenderPassRecording, this,
            &VulkanTriangle::drawFrame, Qt::QueuedConnection);

    // Ensure we start with cleared to black. The squircle's blend mode relies
    // on this.
    win->setColor(Qt::black);
  }
}

void VulkanTriangle::run() {
    initVulkan();
    //mainLoop();
    //cleanup();
}

void VulkanTriangle::initVulkan() {
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();  
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createFramebuffers();
    createCommandPool();
    createCommandBuffer();
    createSemaphores();
    createSyncObjects();
    init_over_ = true;
}

void VulkanTriangle::mainLoop() {
    //qml draw loop
    while(1){
        drawFrame();
    }
}

void VulkanTriangle::cleanup() {
    // 释放信号量
    vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
    vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
    vkDestroyFence(device, inFlightFence, nullptr);
    // 释放commandpool
    vkDestroyCommandPool(device, commandPool, nullptr);
    // 释放framebuffer
    for (auto framebuffer : swapChainFramebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }
    // 释放pipeline
    vkDestroyPipeline(device, graphicsPipeline, nullptr);
    // 释放pipeline
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    // 释放renderpass
    vkDestroyRenderPass(device, renderPass, nullptr);
    // 释放imageview
    for (auto imageView : swapChainImageViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }
    vkDestroySwapchainKHR(device, swapChain, nullptr);
    vkDestroyDevice(device, nullptr);
    DestroyDebugUtilsMessengerEXT(vkinstance, callback, nullptr);
    vkDestroySurfaceKHR(vkinstance, surface, nullptr);
    vkDestroyInstance(vkinstance, nullptr);
}

//01 配置扩展
std::vector<const char*> VulkanTriangle::getRequiredExtensions() {
    std::vector<const char*> charextensions;
    // 查询可用扩展数量
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    // 为扩展名分配内存
    std::vector<VkExtensionProperties> extensions(extensionCount);

    // 查询扩展详细信息
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount,
                                           extensions.data());

    // 打印所有可用扩展
    for (const auto& extension : extensions) {
        qDebug() << "\t" << extension.extensionName;
    }

    charextensions.push_back("VK_KHR_surface");
    charextensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
    charextensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    return charextensions;
}

bool VulkanTriangle::CheckValidationLayerSupport(){
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

//02 校验层
static VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
              VkDebugUtilsMessageTypeFlagsEXT messageType,
              const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
              void* pUserData) {
    qDebug() << "validation layer: " << pCallbackData->pMessage;
    return VK_FALSE;
}

void VulkanTriangle::setupDebugMessenger() {
    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
    createInfo.pUserData = nullptr;  // Optional
    createInfo.pNext = nullptr;
    createInfo.flags = 0;

    if (CreateDebugUtilsMessengerEXT(vkinstance, &createInfo, nullptr,
                                     &callback) != VK_SUCCESS) {
        qFatal("failed to set up debug messenger!");
    }
}

VkResult VulkanTriangle::CreateDebugUtilsMessengerEXT(
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

void VulkanTriangle::DestroyDebugUtilsMessengerEXT(
    VkInstance instance,
    VkDebugUtilsMessengerEXT debugMessenger,
    const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

//03 选择物理设备
void VulkanTriangle::pickPhysicalDevice() {
    // 查询可用设备数量
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(vkinstance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        qFatal("failed to find GPUs with Vulkan support!");
    }
    // 为设备名分配内存
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(vkinstance, &deviceCount, devices.data());

    for(const auto & device : devices){
        if (isDeviceSuitable(device)) {
            physicalDevice = device;
            break;
        }
    }

    if(physicalDevice == VK_NULL_HANDLE){
        qFatal("failed to find a suitable GPU!");
    }

}

//03 判断设备是否可用
bool VulkanTriangle::isDeviceSuitable(VkPhysicalDevice device) {
    QueueFamilyIndices indices = findQueueFamilies(device);

    bool extensionsSupported = checkDeviceExtensionSupport(device);

    bool swapChainAdequate = false;
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
        swapChainAdequate = !swapChainSupport.formats.empty() &&
                            !swapChainSupport.presentModes.empty();
    }

    return indices.isComplete() && extensionsSupported && swapChainAdequate;
}

//03 判断设备是否支持扩展
bool VulkanTriangle::checkDeviceExtensionSupport(VkPhysicalDevice device) {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    //    我们将所需的扩展保存在一个集合中，然后枚举所有可用的扩展，
    // 将集合中的扩展剔除，最后，如果这个集合中的元素为 0 ，说明我们
    // 所需的扩展全部都被满足。
    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for(const auto & extension : availableExtensions){
        requiredExtensions.erase(extension.extensionName);
    }
    return requiredExtensions.empty();
}

//03 获取设备队列
VulkanTriangle::QueueFamilyIndices VulkanTriangle::findQueueFamilies(
    VkPhysicalDevice device) {
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for(const auto & queueFamily : queueFamilies){
        // 判断是否支持图形队列
        if(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT){
            indices.graphicsFamily = i;
        }

        VkBool32 isPresentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, indices.graphicsFamily, surface, &isPresentSupport);
        if(isPresentSupport){
            indices.presentFamily = i;
        }

        if(indices.isComplete()){
            break;
        }

        i++;
    }


    return indices;
} 

//04 创建逻辑设备
void VulkanTriangle::createLogicalDevice(){
    // 获取队列
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = indices.graphicsFamily;
    queueCreateInfo.queueCount = 1;

    // 优先级
    float queuePriority = 1.0f;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    VkPhysicalDeviceFeatures deviceFeatures{};

    // 创建逻辑设备
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = &queueCreateInfo;
    createInfo.queueCreateInfoCount = 1;
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = 0;

    createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();
    
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if(vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS){
        qFatal("failed to create logical device!");
    }

    // 获取队列
    vkGetDeviceQueue(device, indices.graphicsFamily, 0, &graphicsQueue);
    vkGetDeviceQueue(device, indices.presentFamily, 0, &presentQueue);
}

//05 创建表面
void VulkanTriangle::createSurface() {
    VkWin32SurfaceCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.hwnd = reinterpret_cast<HWND>(window()->winId());
    createInfo.hinstance = GetModuleHandle(nullptr);

    // Get the function pointer for vkCreateWin32SurfaceKHR
    PFN_vkCreateWin32SurfaceKHR pfn_vkCreateWin32SurfaceKHR =
        reinterpret_cast<PFN_vkCreateWin32SurfaceKHR>(
            vkGetInstanceProcAddr(vkinstance, "vkCreateWin32SurfaceKHR"));

    // Check if the function pointer is valid
    if (!pfn_vkCreateWin32SurfaceKHR) {
        qFatal("Failed to get vkCreateWin32SurfaceKHR function pointer.");
    }

    // Use the function pointer to create the surface
    VkResult result =
        pfn_vkCreateWin32SurfaceKHR(vkinstance, &createInfo, nullptr, &surface);

    if (result != VK_SUCCESS) {
        qFatal("failed to create window surface!");
    }
}

//06 选择交换链
VulkanTriangle::SwapChainSupportDetails VulkanTriangle::querySwapChainSupport(
    VkPhysicalDevice device) {
    SwapChainSupportDetails details;

    // 获取表面能力
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface,
                                              &details.capabilities);

    // 获取表面格式
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount,
                                         nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount,
                                             details.formats.data());
    }

    // 获取表面支持的呈现模式
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface,
                                              &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            device, surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

//06 选择交换链格式
VkSurfaceFormatKHR VulkanTriangle::chooseSwapSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR>& availableFormats){
    // 如果只有一个格式，且为VK_FORMAT_UNDEFINED，则表示支持所有格式
    if(availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED){
        return {VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
    }

    for(const auto & availableFormat : availableFormats){
        if(availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR){
            return availableFormat;
        }
    }
}

//06 选择交换链呈现模式
VkPresentModeKHR VulkanTriangle::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>&
availablePresentModes){
    // VK_PRESENT_MODE_FIFO_KHR：垂直同步，这是最常用的呈现模式，它会等待垂直
    // 同步信号，也就是等待屏幕刷新，这样就能保证每次显示的图像都是最新的
    // VK_PRESENT_MODE_IMMEDIATE_KHR：立即呈现，这个模式下，如果应用程序提交的
    // 图像还没有显示，那么它会被丢弃，这种模式下，图像可能会出现撕裂的情况
    // VK_PRESENT_MODE_MAILBOX_KHR：邮差模式，这个模式下，如果应用程序提交的图
    // 像还没有显示，那么它会被丢弃，但是与立即呈现模式不同的是，它会保留最新的
    // 图像，这样就不会出现撕裂的情况
    // VK_PRESENT_MODE_FIFO_RELAXED_KHR：垂直同步，但是如果应用程序提交的图像
    // 比较晚，那么它会等待下一个垂直同步信号，这样就能保证每次显示的图像都是最
    // 新的
    for(const auto & availablePresentMode : availablePresentModes){
        if(availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR){
            return availablePresentMode;
        }else if(availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR){
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

//06 选择交换链分辨率
VkExtent2D VulkanTriangle::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities){
    // 如果当前分辨率为UINT32_MAX，则表示支持任意分辨率
    if(capabilities.currentExtent.width != UINT32_MAX){
        return capabilities.currentExtent;
    } else {
        // 如果不支持任意分辨率，则需要自己指定分辨率
        VkExtent2D actualExtent = {800, 600};

        actualExtent.width =
            max(capabilities.minImageExtent.width,
                min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height =
            max(capabilities.minImageExtent.height,
                min(capabilities.maxImageExtent.height, actualExtent.height));

        return actualExtent;
    }
}

//06 创建交换链
void VulkanTriangle::createSwapChain(){
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

    // 交换链中的图像数量，一般比最小值多1，但是不能超过最大值
    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if(swapChainSupport.capabilities.maxImageCount > 0 &&
        imageCount > swapChainSupport.capabilities.maxImageCount){
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    // 交换律绑定表面
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    // 设置交换律图形的信息
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    // 设置交换链图形队列族
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily,
                                     indices.presentFamily};

    // 如果图形队列族和呈现队列族不同，则需要设置图形共享模式
    // VK_SHARING_MODE_EXCLUSIVE：独占模式，图形队列族和呈现队列族不同，且不同队列族
    // 不能同时访问交换链图像，这时需要设置独占模式
    // VK_SHARING_MODE_CONCURRENT：并发模式，图形队列族和呈现队列族不同，但是不同队列族
    // 可以同时访问交换链图像，这时需要设置并发模式
    if(indices.graphicsFamily != indices.presentFamily){
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    // 设置交换链的转换
    // VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR：图像不需要转换
    // VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR：图像需要旋转90度
    // VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR：图像需要旋转180度
    // VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR：图像需要旋转270度
    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    // 设置交换链的透明度
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    // 设置交换链的旧交换链
    if(vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS){
        qFatal("failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
}


//07 创建图像视图
void VulkanTriangle::createImageViews() {
    swapChainImageViews.resize(swapChainImages.size());

    for (size_t i = 0; i < swapChainImages.size(); i++) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = swapChainImages[i];
        // 设置图像视图的类型
        // VK_IMAGE_VIEW_TYPE_1D：1D图像
        // VK_IMAGE_VIEW_TYPE_2D：2D图像
        // VK_IMAGE_VIEW_TYPE_3D：3D图像
        // VK_IMAGE_VIEW_TYPE_CUBE：立方体图像
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = swapChainImageFormat;
        // 设置图像视图的通道映射
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        // 设置图像视图的子资源范围
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
            qFatal("failed to create image views!");
        }
    }
}

std::vector<char> readFile(const std::string& filename) {
    // 以二进制方式打开文件
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        qFatal("failed to open file!");
    }

    // 获取文件大小
    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    // 读取文件内容
    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
}

//08 创建着色器模块
VkShaderModule VulkanTriangle::createShaderModule(
    const std::vector<char>& code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if(vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS){
        qFatal("failed to create shader module!");
    }

    return shaderModule;
}

//08 创建渲染通道
void VulkanTriangle::createGraphicsPipeline(){
    // 读取着色器文件
    auto vertShaderCode = readFile("./shader/triangle.vert.spv");
    auto fragShaderCode = readFile("./shader/triangle.frag.spv");

    // 创建着色器模块
    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

    // 创建着色器阶段
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    // 设置着色器模块 
    vertShaderStageInfo.module = vertShaderModule;
    // 设置着色器入口函数  如果着色器模块中有多个入口函数，可以通过这个参数指定使用哪个入口函数
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    // 设置着色器阶段
    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    // 设置顶点输入
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    // 设置顶点输入绑定
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.pVertexBindingDescriptions = nullptr;
    // 设置顶点输入属性
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.pVertexAttributeDescriptions = nullptr;

    // 设置图元输入
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    // 设置图元拓扑
    // VK_PRIMITIVE_TOPOLOGY_POINT_LIST：点列表
    // VK_PRIMITIVE_TOPOLOGY_LINE_LIST：线列表
    // VK_PRIMITIVE_TOPOLOGY_LINE_STRIP：线带
    // VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST：三角形列表
    // VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP：三角形带
    // VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN：三角形扇
    // ...
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    // 设置是否重用第一个顶点来构造后续的图元
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // 设置视口
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)swapChainExtent.width;
    viewport.height = (float)swapChainExtent.height;
    // 设置深度范围
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    // 设置裁剪区域
    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapChainExtent;

    // 设置视口状态
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    // 设置视口数量
    viewportState.viewportCount = 1;
    // 设置视口
    viewportState.pViewports = &viewport;
    // 设置裁剪区域数量
    viewportState.scissorCount = 1;
    // 设置裁剪区域
    viewportState.pScissors = &scissor;

    // 设置光栅化
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    // 设置是否开启多边形填充模式
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    // 设置是否开启线宽
    rasterizer.lineWidth = 1.0f;
    // 设置是否开启多边形背面剔除
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    // 设置是否开启多边形正面剔除
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    // 设置是否开启深度裁剪
    rasterizer.depthClampEnable = VK_FALSE;
    // 设置是否开启多边形偏移
    rasterizer.depthBiasEnable = VK_FALSE;
    // 设置多边形偏移系数
    rasterizer.depthBiasConstantFactor = 0.0f; // Optional
    // 设置多边形偏移系数
    rasterizer.depthBiasClamp = 0.0f; // Optional
    // 设置多边形偏移系数
    rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

    // 设置多重采样
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    // 设置采样数量
    multisampling.sampleShadingEnable = VK_FALSE;
    // 设置采样数量
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    // 设置采样数量
    multisampling.minSampleShading = 1.0f; // Optional
    // 设置采样掩码
    multisampling.pSampleMask = nullptr; // Optional
    // 设置是否开启多边形偏移
    multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
    // 设置是否开启多边形偏移
    multisampling.alphaToOneEnable = VK_FALSE; // Optional


    // 设置颜色混合
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

    
    // 设置颜色混合状态
    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    // 设置是否开启颜色混合
    colorBlending.logicOpEnable = VK_FALSE;
    // 设置逻辑运算符
    colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
    // 设置混合附件数量
    colorBlending.attachmentCount = 1;
    // 设置混合附件
    colorBlending.pAttachments = &colorBlendAttachment;
    // 设置混合常量
    colorBlending.blendConstants[0] = 0.0f; // Optional
    colorBlending.blendConstants[1] = 0.0f; // Optional
    colorBlending.blendConstants[2] = 0.0f; // Optional
    colorBlending.blendConstants[3] = 0.0f; // Optional

    // 设置动态状态
    VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT,
                                      VK_DYNAMIC_STATE_SCISSOR};

    // 设置动态状态
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    // 设置动态状态数量
    dynamicState.dynamicStateCount = 2;
    // 设置动态状态
    dynamicState.pDynamicStates = dynamicStates;

    // 设置管线布局
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    // 设置管线布局描述集数量
    pipelineLayoutInfo.setLayoutCount = 0; // Optional
    // 设置管线布局描述集
    pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
    // 设置管线布局推送常量数量
    pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
    // 设置管线布局推送常量
    pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional
    
    // 创建管线布局
    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        qFatal("failed to create pipeline layout!");
    }
    
    // 设置管线
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    // 设置管线阶段数量
    pipelineInfo.stageCount = 2;
    // 设置管线阶段
    pipelineInfo.pStages = shaderStages;
    // 设置顶点输入
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    // 设置输入拓扑
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    // 设置视口
    pipelineInfo.pViewportState = &viewportState;
    // 设置光栅化
    pipelineInfo.pRasterizationState = &rasterizer;
    // 设置多重采样
    pipelineInfo.pMultisampleState = &multisampling;
    // 设置颜色混合
    pipelineInfo.pColorBlendState = &colorBlending;
    // 设置动态状态
    pipelineInfo.pDynamicState = &dynamicState;
    // 设置管线布局
    pipelineInfo.layout = pipelineLayout;
    // 设置渲染通道
    pipelineInfo.renderPass = renderPass;
    // 设置子通道索引
    pipelineInfo.subpass = 0;
    // 设置基础管线
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
    // 设置基础管线索引
    pipelineInfo.basePipelineIndex = -1; // Optional

    // 创建管线
    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
        qFatal("failed to create graphics pipeline!");
    }


    // 销毁着色器模块
    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);
}

//09 创建渲染通道
void VulkanTriangle::createRenderPass(){
    // 设置颜色附件
    VkAttachmentDescription colorAttachment{};
    // 设置颜色附件格式
    colorAttachment.format = swapChainImageFormat;
    // 设置颜色附件采样数量
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    // 设置颜色附件加载操作
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    // 设置颜色附件存储操作
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    // 设置颜色附件模板加载操作
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    // 设置颜色附件模板存储操作
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    // 设置颜色附件初始布局
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    // 设置颜色附件最终布局
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // 设置引用颜色附件
    VkAttachmentReference colorAttachmentRef{};
    // 设置引用颜色附件索引
    colorAttachmentRef.attachment = 0;
    // 设置引用颜色附件布局
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // 设置子通道
    VkSubpassDescription subpass{};
    // 设置子通道标志
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    // 设置子通道颜色附件数量
    subpass.colorAttachmentCount = 1;
    // 设置子通道颜色附件
    subpass.pColorAttachments = &colorAttachmentRef;

    // 设置渲染通道创建信息
    VkRenderPassCreateInfo renderPassInfo{};
    // 设置渲染通道结构体类型
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    // 设置渲染通道颜色附件数量
    renderPassInfo.attachmentCount = 1;
    // 设置渲染通道颜色附件
    renderPassInfo.pAttachments = &colorAttachment;
    // 设置渲染通道子通道数量
    renderPassInfo.subpassCount = 1;
    // 设置渲染通道子通道
    renderPassInfo.pSubpasses = &subpass;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    // 创建渲染通道
    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        qFatal("failed to create render pass!");
    }
}

const int MAX_FRAMES_IN_FLIGHT = 2;

//10 创建帧缓冲
void VulkanTriangle::createFramebuffers() {
  swapChainFramebuffers.resize(swapChainImageViews.size());

  for (size_t i = 0; i < swapChainImageViews.size(); i++) {
    VkImageView attachments[] = {swapChainImageViews[i]};

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = attachments;
    framebufferInfo.width = swapChainExtent.width;
    framebufferInfo.height = swapChainExtent.height;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(device, &framebufferInfo, nullptr,
                            &swapChainFramebuffers[i]) != VK_SUCCESS) {
      qFatal("failed to create framebuffer!");
    }
  }
}

//11 创建命令池
void VulkanTriangle::createCommandPool() {
    // 设置队列族索引
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

    // 设置命令池创建信息
    VkCommandPoolCreateInfo poolInfo{};
    // 设置命令池结构体类型
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    // 设置命令池队列族索引
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
    // 设置命令池标志
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    // 创建命令池
    if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
        qFatal("failed to create command pool!");
    }
}

//12 创建命令缓冲
void VulkanTriangle::createCommandBuffer(){
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer) != VK_SUCCESS) {
      qFatal("failed to allocate command buffers!");
    }
}

void VulkanTriangle::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
      qFatal("failed to begin recording command buffer!");
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapChainExtent;

    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float) swapChainExtent.width;
        viewport.height = (float) swapChainExtent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = swapChainExtent;
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
      qFatal("failed to record command buffer!");
    }
}
//13 创建同步对象
void VulkanTriangle::createSemaphores(){
    // 设置信号量创建信息
    VkSemaphoreCreateInfo semaphoreInfo{};
    // 设置信号量结构体类型
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    // 创建信号量
    if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
        vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS) {

        qFatal("failed to create semaphores!");
    }
}

void VulkanTriangle::createSyncObjects() {
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
        vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS ||
        vkCreateFence(device, &fenceInfo, nullptr, &inFlightFence) != VK_SUCCESS) {
      qFatal("failed to create synchronization objects for a frame!");
    }

}
//14 绘制帧
void VulkanTriangle::drawFrame() {
  if (!init_over_) {
    return;
  }
  window()->beginExternalCommands();
  vkWaitForFences(device, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
  vkResetFences(device, 1, &inFlightFence);
  uint32_t imageIndex;
  vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphore,
                        VK_NULL_HANDLE, &imageIndex);

  vkResetCommandBuffer(commandBuffer, /*VkCommandBufferResetFlagBits*/ 0);
  recordCommandBuffer(commandBuffer, imageIndex);

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  VkSemaphore waitSemaphores[] = {imageAvailableSemaphore};
  VkPipelineStageFlags waitStages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;

  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  VkSemaphore signalSemaphores[] = {renderFinishedSemaphore};
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores;

  if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence) !=
      VK_SUCCESS) {
    qFatal("failed to submit draw command buffer!");
  }

  VkPresentInfoKHR presentInfo{};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = signalSemaphores;

  VkSwapchainKHR swapChains[] = {swapChain};
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapChains;

  presentInfo.pImageIndices = &imageIndex;

  vkQueuePresentKHR(presentQueue, &presentInfo);
  window()->endExternalCommands();
}

void VulkanTriangle::populateDebugMessengerCreateInfo(
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

//00 创建实例
void VulkanTriangle::createInstance() {
  if (enableValidationLayers && !CheckValidationLayerSupport()) {
    qFatal("validation layers requested, but not available!");
    return;
  }
  //    填写应用程序信息，这些信息的填写不是必须的，但填写的信
  // 息可能会作为驱动程序的优化依据，让驱动程序进行一些特殊的优化。比
  // 如，应用程序使用了某个引擎，驱动程序对这个引擎有一些特殊处理，这
  // 时就可能有很大的优化提升
  VkApplicationInfo appInfo{};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = "Hello Triangle";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "NoEngine";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_0;

  // 告诉 Vulkan 的驱动程序需要使用的全局扩展和校验层
  VkInstanceCreateInfo creatInfo{};
  creatInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  creatInfo.pApplicationInfo = &appInfo;
  creatInfo.flags = 0;

  // 指定扩展
  auto extensionsVec = getRequiredExtensions();
  creatInfo.enabledExtensionCount = static_cast<uint32_t>(extensionsVec.size());
  creatInfo.ppEnabledExtensionNames = extensionsVec.data();

  // 指定校验层
  VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
  creatInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
  creatInfo.ppEnabledLayerNames = validationLayers.data();

  populateDebugMessengerCreateInfo(debugCreateInfo);
  creatInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;

  // 创建实例
  VkResult result = vkCreateInstance(&creatInfo, nullptr, &vkinstance);
  if (result != VK_SUCCESS) {
    qFatal("failed to create instance!");
  }
}


#include "vulkansquircle.moc"