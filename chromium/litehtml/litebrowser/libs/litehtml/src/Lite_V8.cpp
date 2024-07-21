#include "Lite_V8.h"
#include "v8-initialization.h"
#include "v8-context.h"
#include "v8-initialization.h"
#include "v8-isolate.h"
#include "v8-local-handle.h"
#include "v8-primitive.h"
#include "v8-script.h"

void Lite_V8::Init(const std::string& path) {
  return;






  // Initialize V8
  v8::V8::InitializeICUDefaultLocation(path.c_str());
  v8::V8::InitializeExternalStartupData(path.c_str());
  platform = v8::platform::NewDefaultPlatform();
  v8::V8::InitializePlatform(platform.get());
  v8::V8::Initialize();

  // Create a new Isolate and make it the current one.
  v8::Isolate::CreateParams create_params;
  create_params.array_buffer_allocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator();
  isolate = v8::Isolate::New(create_params);
}

void Lite_V8::ExecuteScript(const std::string& script) {
  v8::Isolate::Scope isolate_scope(isolate);
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::ObjectTemplate> global = v8::ObjectTemplate::New(isolate);
  v8::Local<v8::Context> context = v8::Context::New(isolate, nullptr, global);
  v8::Context::Scope context_scope(context);

  v8::Local<v8::String> source =
    v8::String::NewFromUtf8(isolate, script.c_str(),
      v8::NewStringType::kNormal).ToLocalChecked();
  v8::Local<v8::Script> compiled_script =
    v8::Script::Compile(context, source).ToLocalChecked();

  // Run the script!
  v8::Local<v8::Value> result = compiled_script->Run(context).ToLocalChecked();

  v8::String::Utf8Value utf8(isolate, result);
  printf("%s\n", *utf8);
}