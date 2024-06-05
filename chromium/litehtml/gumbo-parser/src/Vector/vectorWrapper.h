#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct GumboVector {
  void** data;
  unsigned int length;
  unsigned int capacity;
  void* cpp_vector;  // 指向 C++ 实现的指针
} GumboVector;

struct GumboInternalParser;

extern const GumboVector kGumboEmptyVector;

void gumbo_vector_init(struct GumboInternalParser* parser,
    size_t initial_capacity, GumboVector* vector);
void gumbo_vector_destroy(
    struct GumboInternalParser* parser, GumboVector* vector);
void gumbo_vector_add(
    struct GumboInternalParser* parser, void* element, GumboVector* vector);
void* gumbo_vector_pop(struct GumboInternalParser* parser, GumboVector* vector);
int gumbo_vector_index_of(GumboVector* vector, const void* element);
void gumbo_vector_insert_at(struct GumboInternalParser* parser, void* element,
    unsigned int index, GumboVector* vector);
void gumbo_vector_remove(
    struct GumboInternalParser* parser, void* node, GumboVector* vector);
void* gumbo_vector_remove_at(struct GumboInternalParser* parser,
    unsigned int index, GumboVector* vector);

#ifdef __cplusplus
}
#endif
