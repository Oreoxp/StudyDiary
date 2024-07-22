#pragma once
#include <string>
#include "v8.h"
#include "libplatform/libplatform.h"

class DomElement {
public:
  std::string id;
  std::string innerText;

  DomElement(const std::string& id) : id(id) {}
  DomElement(){}
};

class DomInterface {
private:
  std::unordered_map<std::string, DomElement> elements;

public:
  DomElement* getElementById(const std::string& id);
  void setInnerText(const std::string& id, const std::string& text);
};

v8::Local<v8::ObjectTemplate> CreateDomTemplate(v8::Isolate* isolate, DomInterface* dom);

class Lite_V8 {
public:
  Lite_V8();
  ~Lite_V8();

  void Init(const std::string& path, int type);
  void ExecuteScript(std::string script);

private:
  std::unique_ptr<v8::Platform> platform;
  v8::Isolate* isolate;
  v8::Global<v8::Context> context;
  std::vector<std::string> scripts;
  DomInterface dom_;
};
