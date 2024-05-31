#include "gumbo.h"
#include <ctime>
#include <fstream>
#include <iostream>
#include <string>
#include <windows.h>

const int kNumReps = 100;

int main(int argc, char** argv) {
  if (argc != 1) {
    std::cout << "Usage: benchmarks\n";
    exit(EXIT_FAILURE);
  }

  std::string directory = ".\\*.html";
  WIN32_FIND_DATA findFileData;
  HANDLE hFind = FindFirstFile(directory.c_str(), &findFileData);

  if (hFind == INVALID_HANDLE_VALUE) {
    std::cout << "Couldn't find 'benchmarks' directory. Run from root of "
                 "distribution.\n";
    exit(EXIT_FAILURE);
  }

  do {
    std::string filename(findFileData.cFileName);
    std::string full_filename = filename;
    std::ifstream in(full_filename.c_str(), std::ios::in | std::ios::binary);
    if (!in) {
      std::cout << "File " << full_filename << " couldn't be read!\n";
      exit(EXIT_FAILURE);
    }

    std::string contents;
    in.seekg(0, std::ios::end);
    contents.resize(in.tellg());
    in.seekg(0, std::ios::beg);
    in.read(&contents[0], contents.size());
    in.close();

    clock_t start_time = clock();
    for (int i = 0; i < kNumReps; ++i) {
      GumboOutput* output = gumbo_parse(contents.c_str());
      gumbo_destroy_output(&kGumboDefaultOptions, output);
    }
    clock_t end_time = clock();
    std::cout << filename << ": "
              << (1000000 * (end_time - start_time) /
                     (kNumReps * CLOCKS_PER_SEC))
              << " microseconds.\n";

  } while (FindNextFile(hFind, &findFileData) != 0);

  FindClose(hFind);
}
