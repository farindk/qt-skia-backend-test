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

#ifndef DRAWINGWIDGET_SKIA_H
#define DRAWINGWIDGET_SKIA_H

#include <QWidget>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>

#include <core/SkSurface.h>
#include <gpu/ganesh/gl/GrGLDirectContext.h>

#ifdef _WIN32 // TODO(skia): how can we test the skia version?
#include "gpu/GrDirectContext.h"
#else
#include "gpu/ganesh/GrDirectContext.h"
#endif


class DrawingWidget_Skia_GL : public QOpenGLWidget, protected QOpenGLFunctions
{
Q_OBJECT

public:
  DrawingWidget_Skia_GL();

protected:
  void initializeGL() override;

  void resizeGL(int w, int h) override;

  void paintGL() override;

  void resizeEvent(QResizeEvent* e) override;


private:
  int mViewWidth = 0, mViewHeight = 0;

  sk_sp<GrDirectContext> m_grContext;
  sk_sp<SkSurface> m_surface;

  void createSurface(int w, int h);
};

#endif
