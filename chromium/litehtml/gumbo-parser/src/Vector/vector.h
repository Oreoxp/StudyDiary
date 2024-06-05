#pragma once
#include <cassert>
#include <cstring>
#include <vector>

namespace gumbo {

class GumboVector {
 public:
  GumboVector(size_t initial_capacity = 0) {
    if (initial_capacity > 0) {
      data.reserve(initial_capacity);
    }
  }

  ~GumboVector() = default;

  void add(void* element) {
    enlarge_if_full();
    data.push_back(element);
  }

  void* pop() {
    if (data.empty()) {
      return nullptr;
    }
    void* element = data.back();
    data.pop_back();
    return element;
  }

  int index_of(const void* element) const {
    for (size_t i = 0; i < data.size(); ++i) {
      if (data[i] == element) {
        return static_cast<int>(i);
      }
    }
    return -1;
  }

  void insert_at(void* element, size_t index) {
    assert(index >= 0 && index <= data.size());
    enlarge_if_full();
    data.insert(data.begin() + index, element);
  }

  void remove(void* element) {
    int index = index_of(element);
    if (index != -1) {
      remove_at(index);
    }
  }

  void* remove_at(size_t index) {
    assert(index >= 0 && index < data.size());
    void* result = data[index];
    data.erase(data.begin() + index);
    return result;
  }

  void* at(size_t index) {
    assert(index >= 0 && index < data.size());
    void* result = data[index];
    return result;
  }

  int size() { return data.size(); }

  int capacity() { return data.capacity();}

  std::vector<void*> data;
 private:

  void enlarge_if_full() {
    if (data.size() >= data.capacity()) {
      data.reserve(data.capacity() ? data.capacity() * 2 : 2);
    }
  }
};

}  // namespace gumbo