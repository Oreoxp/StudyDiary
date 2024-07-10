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
#include "include/codec/SkCodec.h"
#include "include/encode/SkPngEncoder.h"
#include "include/ports/SKTypeface_win.h"
#include "include/codec/SkJpegDecoder.h"
#include "include/core/SkBitmap.h"
#include "include/core/SkTextBlob.h"

#include <Windows.h>

std::string toUtf8(const char* gb2312) {
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
}

void func1_showText(SkPaint& paint, SkCanvas* canvas) {
  sk_sp<SkFontMgr> mgr = SkFontMgr_New_DirectWrite();
  sk_sp<SkTypeface> face = mgr->matchFamilyStyle("Microsoft YaHei", SkFontStyle());
  if (!face) {
    printf("Cannot open typeface\n");
  }

  SkFont font(face, 14);
  paint.setColor(SK_ColorGREEN);

  std::string str = toUtf8("中文测试：一二三四，123456");
  canvas->drawString(str.c_str(), 10, 25, font, paint);
}

int func1_showText2(SkPaint& paint, SkCanvas* canvas, int x, int y, std::string txt) {
  sk_sp<SkFontMgr> mgr = SkFontMgr_New_DirectWrite();
  sk_sp<SkTypeface> face = mgr->matchFamilyStyle("Georgia", SkFontStyle());
  if (!face) {
    printf("Cannot open typeface\n");
  }

  SkFont font(face, 14);
  paint.setColor(SK_ColorGREEN);

  std::string str = toUtf8(txt.c_str());


  SkRect bounds;
  float width = font.measureText(str.c_str(), str.size(), SkTextEncoding::kUTF8, &bounds);

  // 创建 SkTextBlob
  auto blob = SkTextBlob::MakeFromString(str.c_str(), font);
  canvas->drawTextBlob(blob, x, y, paint);
  return width;
}

void func2_showCircle(SkPaint& paint, SkCanvas* canvas) {
  paint.setColor(SK_ColorBLACK);
  canvas->drawCircle({ 100,100 }, 20, paint);
}

struct imginfo {
  int x = 0;
  int y = 0;
  int width;
  int height;
};

void func3_loadImage(SkPaint& paint, SkCanvas* canvas, std::string imgpath, imginfo info) {
  sk_sp<SkData> encoded(SkData::MakeFromFileName(imgpath.c_str()));
  sk_sp<SkImage> img(SkImages::DeferredFromEncodedData(encoded));
  canvas->drawImageRect(img, SkRect::MakeXYWH(info.x, info.y, info.width, info.height),
    SkSamplingOptions({ 1.0f / 3, 1.0f / 3 }));
}

int main(int argc, char** argv) {
  SkFILEWStream output("output.png");
  if (!output.isValid()) {
    printf("Cannot open output file %s\n", argv[1]);
    return 1;
  }
  sk_sp<SkSurface> surface = SkSurfaces::Raster(SkImageInfo::MakeN32(800, 600, kOpaque_SkAlphaType));
  SkCanvas* canvas = surface->getCanvas();

  SkPaint paint;
  paint.setAntiAlias(true);
  canvas->clear(SK_ColorYELLOW);

  func1_showText(paint, canvas);
  func2_showCircle(paint, canvas);
  func3_loadImage(paint, canvas, "pic2.png", { 200, 100, 100 ,100 });
  func3_loadImage(paint, canvas, "pic1.jpeg", { 100, 200, 200 ,100 });

  int x = 0;
  x += func1_showText2(paint, canvas, x, 400, "abc");
  x += func1_showText2(paint, canvas, x, 400, " ");
  x += func1_showText2(paint, canvas, x, 400, "def");
  x += func1_showText2(paint, canvas, x, 400, " ");
  x += func1_showText2(paint, canvas, x, 400, "ghi");

  SkPixmap pixmap;
  if (surface->peekPixels(&pixmap)) {
    if (!SkPngEncoder::Encode(&output, pixmap, {})) {
      printf("Cannot write output\n");
      return 1;
    }
  } else {
    printf("Cannot readback on surface\n");
    return 1;
  }
  return 0;
}
