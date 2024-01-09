#include <iostream>
#include <vector>
#include "JJ_allocator.h"

int * func1(){
  int a = 36;
  int *p = new((&a) + sizeof(int)) int[1];
    
  std::cout << "a=" << &a<< ",p=" << p << std::endl;
  return p;
}

int * func2(){
  int a = 36;
  int *p = new((&a) + sizeof(int)) int[1];
  *p = 10;
  std::cout << "a2=" << &a<< ",p="<< p << std::endl;
  return p;
}

int * func3(int *p){
  int a = 36;
  int ab = 36;
  int ac = 36;
  int ad = 36;
  int ae = 36;
    
  std::cout << "a3=" << &a<< ",p=" << p << ",*p=" << *p << std::endl;
  return nullptr;
}

int main() {
  auto p = func1();
  func3(func2());
  std::cout << "Hello World!" << std::endl;
  int ia[5] = {0, 1, 2, 3, 4};
  std::cout << "ia= " << ia << std::endl;
  std::vector<int, JJ::allocator<int>> v(ia, ia + 5);
  std::cout << "output :";  
  for (auto i : v) {
    std::cout << i << " ";
  }

  std::cout << std::endl;
  
  std::cout << "p= " << p << std::endl;
  *p = 10; 
  std::cout << "p= " << p << std::endl;
  return 0;
}