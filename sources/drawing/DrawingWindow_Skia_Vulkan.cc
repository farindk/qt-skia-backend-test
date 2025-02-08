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
    sInstance.setLayers({"VK_LAYER_KHRONOS_validation"}); // , "VK_LAYER_LUNARG_api_dump"});
    sInstance.setApiVersion(QVersionNumber(1,1,0));
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

  // --- Fill in the Skia backend context

  skgpu::VulkanBackendContext backendContext = {};
  backendContext.fInstance = vkInstance;
  backendContext.fPhysicalDevice = physicalDevice;
  backendContext.fDevice = vkDevice;
  backendContext.fQueue = queue;
  backendContext.fGraphicsQueueIndex = queueFamilyIndex;

  backendContext.fGetProc = my_get_proc; // vkGetInstanceProcAddr;

  // fMaxAPIVersion

  auto version = get_vulkan_instance()->apiVersion();
  backendContext.fMaxAPIVersion = VK_MAKE_API_VERSION(0, version.majorVersion(), version.minorVersion(), version.microVersion());

  // VkPhysicalDeviceFeatures

  auto func_vkGetPhysicalDeviceFeatures = (void (*)(VkPhysicalDevice, VkPhysicalDeviceFeatures*))my_get_proc("vkGetPhysicalDeviceFeatures", vkInstance, vkDevice);

  static VkPhysicalDeviceFeatures deviceFeatures;
  (*func_vkGetPhysicalDeviceFeatures)(physicalDevice, &deviceFeatures);
  deviceFeatures.robustBufferAccess = 0;
  backendContext.fDeviceFeatures = &deviceFeatures;

#if 0
  VulkanBackendContext
  --------------------
    VkInstance                       fInstance = VK_NULL_HANDLE;
    VkPhysicalDevice                 fPhysicalDevice = VK_NULL_HANDLE;
    VkDevice                         fDevice = VK_NULL_HANDLE;
    VkQueue                          fQueue = VK_NULL_HANDLE;
    uint32_t                         fGraphicsQueueIndex = 0;
    // The max api version set here should match the value set in VkApplicationInfo::apiVersion when
    // then VkInstance was created.
    uint32_t                         fMaxAPIVersion = 0;
    const skgpu::VulkanExtensions*   fVkExtensions = nullptr;
    // The client can create their VkDevice with either a VkPhysicalDeviceFeatures or
    // VkPhysicalDeviceFeatures2 struct, thus we have to support taking both. The
    // VkPhysicalDeviceFeatures2 struct is needed so we know if the client enabled any extension
    // specific features. If fDeviceFeatures2 is not null then we ignore fDeviceFeatures. If both
    // fDeviceFeatures and fDeviceFeatures2 are null we will assume no features are enabled.
    const VkPhysicalDeviceFeatures*  fDeviceFeatures = nullptr;
    const VkPhysicalDeviceFeatures2* fDeviceFeatures2 = nullptr;
    // Optional. The client may provide an inplementation of a VulkanMemoryAllocator for Skia to use
    // for allocating Vulkan resources that use VkDeviceMemory.
    sk_sp<VulkanMemoryAllocator>     fMemoryAllocator;
    skgpu::VulkanGetProc             fGetProc;
    Protected                        fProtectedContext = Protected::kNo;
    // Optional callback which will be invoked if a VK_ERROR_DEVICE_LOST error code is received from
    // the driver. Debug information from the driver will be provided to the callback if the
    // VK_EXT_device_fault extension is supported and enabled (VkPhysicalDeviceFaultFeaturesEXT must
    // be in the pNext chain of VkDeviceCreateInfo).
    skgpu::VulkanDeviceLostContext   fDeviceLostContext = nullptr;
    skgpu::VulkanDeviceLostProc      fDeviceLostProc = nullptr;
#endif

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

  int currentFrame;
  //currentFrame = mWindow->currentFrame();
  currentFrame = mWindow->currentSwapChainImageIndex();
  printf("currentFrame: %d\n", currentFrame);

  // Suppose you have the VkImage and its info:
  VkImage image = mWindow->swapChainImage(currentFrame);
  VkFormat imageFormat = mWindow->colorFormat();

  // Construct Skia's VkImageInfo:
  GrVkImageInfo imageInfo;
  imageInfo.fImage = image;
  imageInfo.fImageTiling = VK_IMAGE_TILING_OPTIMAL;
  imageInfo.fImageLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  imageInfo.fFormat = imageFormat;
  // imageInfo.fCurrentQueueFamily =
  // You must also fill in other fields such as fLevelCount, fCurrentQueueFamily,
  // fAlloc, etc. Often you can copy some of these from your initialization or
  // query them from Qt’s VulkanWindow.

#if 0
    GrVkImageInfo
    -------------
    VkImage                           fImage = VK_NULL_HANDLE;
    skgpu::VulkanAlloc                fAlloc;
    VkImageTiling                     fImageTiling = VK_IMAGE_TILING_OPTIMAL;
    VkImageLayout                     fImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkFormat                          fFormat = VK_FORMAT_UNDEFINED;
    VkImageUsageFlags                 fImageUsageFlags = 0;
    uint32_t                          fSampleCount = 1;
    uint32_t                          fLevelCount = 0;
    uint32_t                          fCurrentQueueFamily = VK_QUEUE_FAMILY_IGNORED;
    skgpu::Protected                  fProtected = skgpu::Protected::kNo;
    skgpu::VulkanYcbcrConversionInfo  fYcbcrConversionInfo;
    VkSharingMode                     fSharingMode = VK_SHARING_MODE_EXCLUSIVE;
#endif

  int w = mWindow->swapChainImageSize().width();
  int h = mWindow->swapChainImageSize().height();
  printf("size: %d x %d\n", w, h);

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

  // Flush Skia drawing commands.

  //m_grContext->submit();
  m_grContext->flushAndSubmit();
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
