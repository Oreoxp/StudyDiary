#include <fontconfig/fontconfig.h>
#include "include/core/SkAlphaType.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkColor.h"
#include "include/core/SkImageInfo.h"
#include "include/core/SkRefCnt.h"
#include "include/core/SkTypeface.h"
#include "include/core/SkFont.h"
#include "include/core/SkFontStyle.h"
#include "include/core/SkFontMgr.h"
#include "include/core/SkPixmap.h"
#include "include/core/SkSurface.h"
#include "include/core/SkStream.h"
#include "include/encode/SkPngEncoder.h"
#include "include/ports/SKTypeface_win.h"

#include <Windows.h>

int main(int argc, char** argv) {
  SkFILEWStream output("output.png");
  if (!output.isValid()) {
    printf("Cannot open output file %s\n", argv[1]);
    return 1;
  }
  sk_sp<SkSurface> surface = SkSurfaces::Raster(SkImageInfo::MakeN32(800, 600, kOpaque_SkAlphaType));
  SkCanvas* canvas = surface->getCanvas();
  sk_sp<SkFontMgr> mgr = SkFontMgr_New_DirectWrite();

  sk_sp<SkTypeface> face = mgr->matchFamilyStyle("Microsoft YaHei", SkFontStyle());
  if (!face) {
    printf("Cannot open typeface\n");
    return 1;
  }

  SkFont font(face, 14);
  SkPaint paint;
  paint.setColor(SK_ColorGREEN);
  canvas->clear(SK_ColorYELLOW);
  auto Gb2312_2_UTF8 = [](const char* gb2312)
    {
      int len = MultiByteToWideChar(CP_ACP, 0, gb2312, -1, NULL, 0);
      wchar_t* wstr = new wchar_t[len + 1];
      memset(wstr, 0, len + 1);
      MultiByteToWideChar(CP_ACP, 0, gb2312, -1, wstr, len);
      len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
      char* str = new char[len + 1];
      memset(str, 0, len + 1);
      WideCharToMultiByte(CP_UTF8, 0, wstr, -1, str, len, NULL, NULL);
      if (wstr) delete[] wstr;
      std::string strUtf8 = str;
      if (str) delete[] str;
      return strUtf8;
    };
  std::string str = Gb2312_2_UTF8("中文测试：一二三四，1234");
  canvas->drawString(str.c_str(), 10, 25, font, paint);
  SkPixmap pixmap;
  if (surface->peekPixels(&pixmap)) {
    if (!SkPngEncoder::Encode(&output, pixmap, {})) {
      printf("Cannot write output\n");
      return 1;
    }
  }
  else {
    printf("Cannot readback on surface\n");
    return 1;
  }
  return 0;
}
