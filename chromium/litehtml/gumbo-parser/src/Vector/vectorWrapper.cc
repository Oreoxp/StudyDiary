#include "vector.h"
#include "vectorWrapper.h"

const GumboVector kGumboEmptyVector = {NULL, 0, 0, nullptr};

void gumbo_vector_init(struct GumboInternalParser* parser,
    size_t initial_capacity, GumboVector* vector) {
  vector->length = 0;
  vector->capacity = initial_capacity;
  vector->cpp_vector = new gumbo::GumboVector(initial_capacity);
  vector->data = nullptr;
}

void gumbo_vector_destroy(
    struct GumboInternalParser* parser, GumboVector* vector) {
  if (vector->cpp_vector) {
    delete static_cast<gumbo::GumboVector*>(vector->cpp_vector);
    vector->cpp_vector = nullptr;
  }
}

void gumbo_vector_add(
    struct GumboInternalParser* parser, void* element, GumboVector* vector) {
  auto* cpp_vector = static_cast<gumbo::GumboVector*>(vector->cpp_vector);
  cpp_vector->add(element);
  vector->length = cpp_vector->size();
  vector->capacity = cpp_vector->capacity();
  vector->data = cpp_vector->data.data();
}

void* gumbo_vector_pop(
    struct GumboInternalParser* parser, GumboVector* vector) {
  auto* cpp_vector = static_cast<gumbo::GumboVector*>(vector->cpp_vector);
  void* result = cpp_vector->pop();
  vector->length = cpp_vector->size();
  vector->capacity = cpp_vector->capacity();
  vector->data = cpp_vector->data.data();
  return result;
}

int gumbo_vector_index_of(GumboVector* vector, const void* element) {
  auto* cpp_vector = static_cast<gumbo::GumboVector*>(vector->cpp_vector);
  vector->data = cpp_vector->data.data();
  return cpp_vector->index_of(element);
}

void gumbo_vector_insert_at(struct GumboInternalParser* parser, void* element,
    unsigned int index, GumboVector* vector) {
  auto* cpp_vector = static_cast<gumbo::GumboVector*>(vector->cpp_vector);
  cpp_vector->insert_at(element, index);
  vector->length = cpp_vector->size();
  vector->capacity = cpp_vector->capacity();
  vector->data = cpp_vector->data.data();
}

void gumbo_vector_remove(
    struct GumboInternalParser* parser, void* node, GumboVector* vector) {
  auto* cpp_vector = static_cast<gumbo::GumboVector*>(vector->cpp_vector);
  cpp_vector->remove(node);
  vector->length = cpp_vector->size();
  vector->capacity = cpp_vector->capacity();
  vector->data = cpp_vector->data.data();
}

void* gumbo_vector_remove_at(struct GumboInternalParser* parser,
    unsigned int index, GumboVector* vector) {
  auto* cpp_vector = static_cast<gumbo::GumboVector*>(vector->cpp_vector);
  void* result = cpp_vector->remove_at(index);
  vector->length = cpp_vector->size();
  vector->capacity = cpp_vector->capacity();
  vector->data = cpp_vector->data.data();
  return result;
}
