#pragma once
#include <string>
#include "v8.h"
#include "libplatform/libplatform.h"

class Lite_V8 {
public:
  void Init(const std::string& path);
  void ExecuteScript(const std::string& script);

private:
  std::unique_ptr<v8::Platform> platform;
  v8::Isolate* isolate;
};
