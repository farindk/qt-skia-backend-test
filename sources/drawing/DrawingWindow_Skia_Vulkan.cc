/*
 * Copyright (C) 2025 by Dirk Farin, Kronenstr. 49b, 70174 Stuttgart, Germany
 *
 * 2-Clause BSD
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer
 *    in the documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include <third_party/vulkan/vulkan/vulkan_core.h>
#include "Drawing.h"

#include "DrawingWindow_Skia_Vulkan.h"
#include <QVulkanInstance>
#include <QVulkanDeviceFunctions>

#include <iostream>
#include "NonSkiaVulkanRenderer.h"

#include <core/SkPaint.h>
#include <core/SkCanvas.h>

#ifdef _WIN32 // TODO(skia): how can we test the skia version?
#include "gpu/GrDirectContext.h"
#include <gpu/gl/GrGLInterface.h>
#include <gpu/ganesh/gl/GrGLDirectContext.h>
#include <gpu/ganesh/SkSurfaceGanesh.h>
#else

#include "gpu/ganesh/vk/GrVkTypes.h"
#include "gpu/ganesh/GrDirectContext.h"
#include "gpu/ganesh/vk/GrVkDirectContext.h"
#include <gpu/ganesh/GrBackendSurface.h>
#include <gpu/ganesh/SkSurfaceGanesh.h>
#include <gpu/vk/VulkanBackendContext.h>

#endif


QVulkanInstance* get_vulkan_instance()
{
  static QVulkanInstance sInstance;
  static bool initialized = false;

  if (!initialized) {
    sInstance.setLayers({"VK_LAYER_KHRONOS_validation"});
    if (!sInstance.create()) {
      std::cerr << "Failed to create Vulkan instance: " << sInstance.errorCode() << "\n";
      return nullptr;
    }

    initialized = true;
  }

  return &sInstance;
}


DrawingWindow_Skia_Vulkan::DrawingWindow_Skia_Vulkan(bool skia)
    : mSkia(skia)
{
  auto* vulkan_instance = get_vulkan_instance();
  if (!vulkan_instance) {
    assert(false);
  }
  setVulkanInstance(vulkan_instance);
}


class SkiaRenderer : public QVulkanWindowRenderer {
public:
  SkiaRenderer(QVulkanWindow* w, bool msaa = false);

  void initSkia();

  //Initializes the Vulkan resources needed,
  // the buffers
  // vertex descriptions for the shaders
  // making the shaders, etc
  void initResources() override
  {
    initSkia();
  }

  //Set up resources - only MVP-matrix for now:
  void initSwapChainResources() override;

  //Empty for now - needed since we implement QVulkanWindowRenderer
  void releaseSwapChainResources() override;

  //Release Vulkan resources when program ends
  //Called by Qt
  //void releaseResources() override;

  //Render the next frame
  void startNextFrame() override;

  //Get Vulkan info - just for fun
  //void getVulkanHWInfo();

protected:
  QVulkanWindow* mWindow{nullptr};

  sk_sp<GrDirectContext> m_grContext;
  sk_sp<SkSurface> m_surface;

  void paintVK();
};


SkiaRenderer::SkiaRenderer(QVulkanWindow* w, bool msaa)
    : mWindow(w)
{
  if (msaa) {
    const QVector<int> counts = w->supportedSampleCounts();
    for (int s : counts) {
      std::cout << "Supported sample counts:" << s << "\n";
    }

    for (int s = 16; s >= 4; s /= 2) {
      if (counts.contains(s)) {
        qDebug("Requesting sample count %d", s);
        mWindow->setSampleCount(s);
        break;
      }
    }
  }
}


PFN_vkVoidFunction my_get_proc(
    const char* name, // function name
    VkInstance,  // instance or VK_NULL_HANDLE
    VkDevice)     // device or VK_NULL_HANDLE
{
  auto* f = get_vulkan_instance()->getInstanceProcAddr(name);
  return f;
}


void SkiaRenderer::initSkia()
{
  QVulkanInstance* qVkInst = mWindow->vulkanInstance();
  VkInstance vkInstance = qVkInst->vkInstance();
  VkPhysicalDevice physicalDevice = mWindow->physicalDevice();
  VkDevice vkDevice = mWindow->device();
  VkQueue queue = mWindow->graphicsQueue();
  uint32_t queueFamilyIndex = mWindow->graphicsQueueFamilyIndex();

  // Fill in the Skia backend context
  skgpu::VulkanBackendContext backendContext = {};
  backendContext.fInstance = vkInstance;
  backendContext.fPhysicalDevice = physicalDevice;
  backendContext.fDevice = vkDevice;
  backendContext.fQueue = queue;
  backendContext.fGraphicsQueueIndex = queueFamilyIndex;

  // For fGetProc, you can usually use vkGetInstanceProcAddr.
  // (If you use a custom loader, provide its function pointer here.)
  backendContext.fGetProc = my_get_proc; // vkGetInstanceProcAddr;

  // (Optionally, if needed, set fPhysicalDeviceFeatures, fDeviceFeatures, etc.)

  // Create Skia’s direct context using Vulkan.
  sk_sp<GrDirectContext> grContext = GrDirectContexts::MakeVulkan(backendContext);
  if (!grContext) {
    qFatal("Failed to create Skia GrDirectContext with Vulkan");
  }

  // Store grContext for use during rendering.
  m_grContext = grContext;

  grContext->flushAndSubmit();
}

void SkiaRenderer::initSwapChainResources()
{
  const QSize sz = mWindow->swapChainImageSize();
}


void SkiaRenderer::releaseSwapChainResources()
{
}


void SkiaRenderer::startNextFrame()
{
  paintVK();

  mWindow->frameReady();
  mWindow->requestUpdate(); // render continuously, throttled by the presentation rate
}

void SkiaRenderer::paintVK()
{
  // Retrieve the current Vulkan command buffer and framebuffer info from QVulkanWindow.
  VkCommandBuffer cmdBuffer = mWindow->currentCommandBuffer();
  // You can obtain the current VkImage from the framebuffer, etc.
  // (See the QVulkanWindow documentation for details.)

  int currentFrame = mWindow->currentFrame();
  printf("currentFrame: %d\n", currentFrame);

  // Suppose you have the VkImage and its info:
  VkImage image = mWindow->swapChainImage(currentFrame);
  VkFormat imageFormat = mWindow->colorFormat();

  // Construct Skia's VkImageInfo:
  GrVkImageInfo imageInfo;
  imageInfo.fImage = image;
  imageInfo.fImageLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  imageInfo.fFormat = imageFormat;
  imageInfo.fImageTiling = VK_IMAGE_TILING_OPTIMAL;
  // imageInfo.fCurrentQueueFamily =
  // You must also fill in other fields such as fLevelCount, fCurrentQueueFamily,
  // fAlloc, etc. Often you can copy some of these from your initialization or
  // query them from Qt’s VulkanWindow.

  int w = mWindow->swapChainImageSize().width();
  int h = mWindow->swapChainImageSize().height();
  printf("size: %d x %d\n", w, h);

#if 0
  // Create a GrBackendRenderTarget for Skia:
  GrBackendRenderTarget backendRenderTarget(w, h,
      /*sample count*/ 1,
      /*stencil bits*/ 0,
                                            imageInfo);
#endif

  SkColorType colorType = kBGRA_8888_SkColorType; // or match your VkFormat

  m_surface = SkSurfaces::RenderTarget(
      m_grContext.get(), // GrRecordingContext* context,
      skgpu::Budgeted::kNo, // skgpu::Budgeted budgeted,
      SkImageInfo::Make(w, h, colorType,
                        kOpaque_SkAlphaType), //const SkImageInfo& imageInfo,
      0, // int sampleCount,
      kTopLeft_GrSurfaceOrigin,
      nullptr, // &surfaceProps,
      false, // bool shouldCreateWithMips = false,
      false); // bool isProtected = false);

  // Draw with Skia:
  SkCanvas* canvas = m_surface->getCanvas();
  canvas->clear(SK_ColorWHITE);
  // ... perform additional drawing ...

  draw_skia_scene(canvas);

  //m_grContext->submit();
  m_grContext->flushAndSubmit();

  // Flush Skia drawing commands.
  // m_surface->flushAndSubmit();

  // Optionally, record and submit your Vulkan command buffer that incorporates
  // the Skia drawing operations.
}


QVulkanWindowRenderer* DrawingWindow_Skia_Vulkan::createRenderer()
{
  if (mSkia) {
    return new SkiaRenderer(this, false);
  }
  else {
    return new NonSkiaVulkanRenderer(this, true);
  }
}
