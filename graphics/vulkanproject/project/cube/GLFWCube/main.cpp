#include "Triangle.h"

#define _SILENCE_CXX20_CISO646_REMOVED_WARNING
int main() {
  try {
    startVulkan();
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}