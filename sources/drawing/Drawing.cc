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


#include "Drawing.h"

#include <iostream>
#include "main/MainWindow.h"
#include "SkiaFontManager.h"

#include <core/SkPaint.h>
#include <core/SkCanvas.h>
#include <core/SkFont.h>


void draw_skia_scene(SkCanvas* canvas)
{
  static int cnt = 0;

  SkISize size = canvas->getBaseLayerSize();
  int w = size.width();
  int h = size.height();

  canvas->clear(SK_ColorBLUE);


  // --- draw rotating line

  SkPaint paint;
  paint.setColor(SK_ColorRED);
  paint.setStrokeWidth((w + h) / 100.0f);
  paint.setStyle(SkPaint::kStroke_Style);
  canvas->drawLine(w / 2 + cos(cnt / 100.0) * w * 0.4, h / 2 + sin(cnt / 100.0) * h * 0.4, w / 2, h / 2, paint);


  // --- draw text with increasing font size

  int font_size = cnt / 3;

  auto mgr = get_skia_font_manager();
  SkFont font;
  font.setTypeface(mgr->matchFamilyStyle("FreeSans", {}));
  font.setSize(font_size);

  paint.setColor(SK_ColorWHITE);
  paint.setStyle(SkPaint::kFill_Style);

  std::string text = std::to_string(font_size);
  int text_width = font.measureText(text.c_str(), text.length(), SkTextEncoding::kUTF8);

  canvas->drawSimpleText(text.c_str(), text.length(), SkTextEncoding::kUTF8,
                         (w - text_width) / 2, h * 4 / 5, font, paint);

  cnt++;
}
