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


#include "DrawingWidget_Skia_Software.h"
#include "Drawing.h"

#include <QSurfaceFormat>
#include <QPainter>
#include <QResizeEvent>

#include <iostream>

#include <core/SkPaint.h>
#include <core/SkCanvas.h>

#ifdef _WIN32 // TODO(skia): how can we test the skia version?
#include "gpu/GrDirectContext.h"
#include <gpu/ganesh/SkSurfaceGanesh.h>
#else
#include "gpu/ganesh/GrDirectContext.h"
#include <gpu/ganesh/SkSurfaceGanesh.h>
#endif


DrawingWidget_Skia_Software::DrawingWidget_Skia_Software()
{
}


void DrawingWidget_Skia_Software::resizeEvent(QResizeEvent* e)
{
  mViewWidth = e->size().width();
  mViewHeight = e->size().height();

  SkImageInfo imageInfo = SkImageInfo::Make(mViewWidth, mViewHeight,
                                            kRGBA_8888_SkColorType, kPremul_SkAlphaType);
  m_surface = SkSurfaces::Raster(imageInfo);

  if (!m_surface) {
    qFatal("Failed to create SkSurface");
  }

  update();
}


void DrawingWidget_Skia_Software::paintEvent(QPaintEvent* event)
{
  if (m_surface) {
    SkCanvas* canvas = m_surface->getCanvas();

    draw_skia_scene(canvas);

    SkPixmap pixmap;
    if (!m_surface->peekPixels(&pixmap)) {
      return; // Handle error
    }

    QImage image(
        static_cast<const uchar*>(pixmap.addr()),
        pixmap.width(),
        pixmap.height(),
        pixmap.rowBytes(),
        QImage::Format_RGBA8888);

    QPainter painter(this);

    QRect sourceRect(0, 0, mViewWidth, mViewHeight);
    QRect targetRect(0, 0, QWidget::width(), QWidget::height());

    painter.drawImage(targetRect, image, sourceRect);

    update();
  }
}
