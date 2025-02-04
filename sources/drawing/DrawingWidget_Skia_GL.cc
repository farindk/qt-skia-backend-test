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


#include "DrawingWidget_Skia_GL.h"
#include <QSurfaceFormat>

#include <iostream>
#include "Drawing.h"

#ifdef _WIN32 // TODO(skia): how can we test the skia version?
#include "gpu/GrDirectContext.h"
#include <gpu/gl/GrGLInterface.h>
#include <gpu/ganesh/gl/GrGLDirectContext.h>
#include <gpu/ganesh/SkSurfaceGanesh.h>
#else
#include "gpu/ganesh/GrDirectContext.h"
#include "gpu/ganesh/gl/GrGLInterface.h"
#include <gpu/ganesh/GrBackendSurface.h>
#include <gpu/ganesh/SkSurfaceGanesh.h>
#endif



DrawingWidget_Skia_GL::DrawingWidget_Skia_GL()
{
  setFocusPolicy(Qt::StrongFocus);

  setAttribute(Qt::WA_AcceptTouchEvents, true);

  QSurfaceFormat format;
  format.setSamples(4); // Example: Enable multisampling
  setFormat(format);
}


void DrawingWidget_Skia_GL::initializeGL()
{
    initializeOpenGLFunctions(); // Important!

    auto glinterface = GrGLMakeNativeInterface();
    m_grContext = GrDirectContexts::MakeGL(glinterface);
    if (!m_grContext) {
      qFatal("Failed to create GrDirectContext!");
    }
}


void DrawingWidget_Skia_GL::resizeGL(int w, int h)
{
  mViewWidth = w;
  mViewHeight = h;

  if (m_grContext) {
    createSurface(mViewWidth, mViewHeight);
  }
}


void DrawingWidget_Skia_GL::createSurface(int w, int h)
{
  SkColorType colorType = kRGBA_8888_SkColorType;

  m_surface = nullptr;
  m_surface = SkSurfaces::RenderTarget(
          m_grContext.get(), // GrRecordingContext* context,
          skgpu::Budgeted::kNo, // skgpu::Budgeted budgeted,
          SkImageInfo::Make(w, h, colorType,
                            kOpaque_SkAlphaType), //const SkImageInfo& imageInfo,
          0, // int sampleCount,
          kBottomLeft_GrSurfaceOrigin,
          nullptr, // &surfaceProps,
          false, // bool shouldCreateWithMips = false,
          false); // bool isProtected = false);

  GLenum error = glGetError();
  if (error != GL_NO_ERROR) {
    // Convert error code to string for better debugging
    //const char* errorString = reinterpret_cast<const char*>(gluErrorString(error));
    // qDebug() << "Error string:" << errorString << "\n";
    std::cerr << "error " << error << "\n";
  }

  if (!m_surface) {
    qFatal("Failed to create SkSurface");
  }

  update();
}


void DrawingWidget_Skia_GL::paintGL()
{
  if (m_surface) {
    SkCanvas* canvas = m_surface->getCanvas();

    draw_skia_scene(canvas);

    m_grContext->flush();
  }

  update();
}


void DrawingWidget_Skia_GL::resizeEvent(QResizeEvent* e)
{
  QOpenGLWidget::resizeEvent(e);
}
