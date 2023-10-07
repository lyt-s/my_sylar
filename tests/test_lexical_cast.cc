#include <iostream>
#include <ostream>
#include "sylar/config.h"

int main() {
  std::string str1 = sylar::LexicalCast<int, std::string>()(123);     // ok, str1等于"123"
  int int1 = sylar::LexicalCast<std::string, int>()("123");           // ok, int1等于123
  std::string str2 = sylar::LexicalCast<float, std::string>()(3.14);  // ok，str2等于"3.14"
  float float2 = sylar::LexicalCast<std::string, float>()("3.14");    // ok，float2等于3.14

  std::cout << "int1:" << int1 << std::endl;
  std::cout << "float2:" << float2 << std::endl;

  std::vector<int> v =
      sylar::LexicalCast<std::string, std::vector<int>>()("[1, 2, 3]");  // ok, v等于[1, 2, 3]
  std::string s = sylar::LexicalCast<std::vector<int>, std::string>()(std::vector<int>{1, 2, 3});

  std::cout << "vector:";
  for (auto it : v) {
    std::cout << it;
  }
  std::cout << "string:" << s << std::endl;

  std::vector<std::vector<int>> vv =
      sylar::LexicalCast<std::string, std::vector<std::vector<int>>>()("[[1,2,3],[4,5,6]]");
  std::string ss = sylar::LexicalCast<std::vector<std::vector<int>>, std::string>()(vv);

  std::cout << "vector<vector<int>>:" << std::endl;
  for (auto it : vv) {
    for (auto i : it) {
      std::cout << i;
    }
    std::cout << "\n";
  }
  std::cout << "string:" << ss << std::endl;
  return 0;
}