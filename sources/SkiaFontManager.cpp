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


#include "SkiaFontManager.h"
#include <core/SkStream.h>
#include <core/SkTypeface.h>
#include <ports/SkFontMgr_empty.h>
#include <ports/SkFontMgr_directory.h>


static sk_sp<SkFontMgr> m_font_mgr;

sk_sp<SkFontMgr> get_skia_font_manager()
{
  return m_font_mgr;
}

void set_global_skia_font_manager(sk_sp<SkFontMgr> mgr)
{
  m_font_mgr = mgr;
}

#include <iostream>

void list_fonts(sk_sp<SkFontMgr> fontMgr)
{
  int familyCount = fontMgr->countFamilies();
  for (int i = 0; i < familyCount; ++i) {
    SkString familyName;
    fontMgr->getFamilyName(i, &familyName);
    printf("Font Family: %s\n", familyName.c_str());

    sk_sp<SkFontStyleSet> styleSet = fontMgr->createStyleSet(i);
    if (styleSet) {
      int styleCount = styleSet->count();
      for (int j = 0; j < styleCount; ++j) {
        SkFontStyle style;
        SkString styleName;
        styleSet->getStyle(j, &style, &styleName);
        printf("  Style: %s %d %d\n", styleName.c_str(), style.width(), style.weight());

        auto typeface = styleSet->createTypeface(0);
        int nGlyphs = typeface->countGlyphs();
        printf("  nGlyphs: %d\n", nGlyphs);
      }
    }
  }
}

bool set_global_skia_font_manager_from_fonts_directory(const char* font_directory)
{
  auto mgr = SkFontMgr_New_Custom_Directory(font_directory);

  set_global_skia_font_manager(mgr);
  list_fonts(mgr);

  return true;
}


std::vector<SkString> get_default_font_family_list()
{
  return {SkString{"FreeSans"}};
}
