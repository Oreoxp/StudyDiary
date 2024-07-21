#include "lite_v8.h"

DomElement* DomInterface::getElementById(const std::string& id) {
  if (elements.find(id) == elements.end()) {
    elements[id] = DomElement(id);
  }
  return &elements[id];
}

void DomInterface::setInnerText(const std::string& id, const std::string& text) {
  if (elements.find(id) != elements.end()) {
    elements[id].innerText = text;
  }
}

void GetElementById(const v8::FunctionCallbackInfo<v8::Value>& args) {
  v8::Isolate* isolate = args.GetIsolate();
  v8::HandleScope handle_scope(isolate);

  DomInterface* dom = static_cast<DomInterface*>(args.Data().As<v8::External>()->Value());
  v8::String::Utf8Value utf8(isolate, args[0]);
  std::string id(*utf8);

  DomElement* element = dom->getElementById(id);
  v8::Local<v8::Object> result = v8::Object::New(isolate);
  result->Set(isolate->GetCurrentContext(), v8::String::NewFromUtf8(isolate, "id").ToLocalChecked(), args[0]);
  result->Set(isolate->GetCurrentContext(), v8::String::NewFromUtf8(isolate, "innerText").ToLocalChecked(), v8::String::NewFromUtf8(isolate, element->innerText.c_str()).ToLocalChecked());

  args.GetReturnValue().Set(result);
}

void SetInnerText(const v8::FunctionCallbackInfo<v8::Value>& args) {
  v8::Isolate* isolate = args.GetIsolate();
  v8::HandleScope handle_scope(isolate);

  DomInterface* dom = static_cast<DomInterface*>(args.Data().As<v8::External>()->Value());
  v8::String::Utf8Value utf8_id(isolate, args[0]);
  std::string id(*utf8_id);

  v8::String::Utf8Value utf8_text(isolate, args[1]);
  std::string text(*utf8_text);

  dom->setInnerText(id, text);
}

v8::Local<v8::ObjectTemplate> CreateDomTemplate(v8::Isolate* isolate, DomInterface* dom) {
  v8::Local<v8::ObjectTemplate> dom_template = v8::ObjectTemplate::New(isolate);
  dom_template->Set(isolate, "getElementById", v8::FunctionTemplate::New(isolate, GetElementById, v8::External::New(isolate, dom)));
  dom_template->Set(isolate, "setInnerText", v8::FunctionTemplate::New(isolate, SetInnerText, v8::External::New(isolate, dom)));
  return dom_template;
}

Lite_V8::Lite_V8() : isolate(nullptr) {}

Lite_V8::~Lite_V8() {
  if (isolate) {
    isolate->Dispose();
    v8::V8::Dispose();
  }
}

void Lite_V8::Init(const std::string& path, int type) {
  if (type == 1) {
    return;
  }
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

  // Create a stack-allocated handle scope.
  v8::Isolate::Scope isolate_scope(isolate);
  v8::HandleScope handle_scope(isolate);

  // Create a new context.
  v8::Local<v8::ObjectTemplate> global = v8::ObjectTemplate::New(isolate);
  global->Set(isolate, "dom", CreateDomTemplate(isolate, &dom_));
  v8::Local<v8::Context> local_context = v8::Context::New(isolate, nullptr, global);

  // Store the context in the global handle.
  context.Reset(isolate, local_context);
}

void Lite_V8::ExecuteScript(std::string script) {
  script = R"(
        dom.setInnerText('output', 'Document is loaded');
        console.log(dom.getElementById('output').innerText);
    )";

  v8::Isolate::Scope isolate_scope(isolate);
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> local_context = context.Get(isolate);
  v8::Context::Scope context_scope(local_context);

  // Compile and run the script.
  v8::Local<v8::String> source = v8::String::NewFromUtf8(isolate, script.c_str(), v8::NewStringType::kNormal).ToLocalChecked();
  v8::Local<v8::Script> compiled_script;

  if (!v8::Script::Compile(local_context, source).ToLocal(&compiled_script)) {
    fprintf(stderr, "Failed to compile script\n");
    return;
  }
  v8::Local<v8::Value> result = compiled_script->Run(local_context).ToLocalChecked();
  v8::String::Utf8Value utf8(isolate, result);
}