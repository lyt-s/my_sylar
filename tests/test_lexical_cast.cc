#include <yaml-cpp/yaml.h>
#include <iostream>
#include <ostream>
#include <string>
#include <vector>
#include "boost/lexical_cast.hpp"
// #include "sylar/config.h"

template <typename F, typename T>
class LexicalCast {
 public:
  T operator()(const F &v) { return boost::lexical_cast<T>(v); }
};

template <class T>
class LexicalCast<std::string, std::vector<T>> {
 public:
  std::vector<T> operator()(const std::string &v) {
    YAML::Node node = YAML::Load(v);
    typename std::vector<T> vec;
    std::stringstream ss;
    for (size_t i = 0; i < node.size(); ++i) {
      // 清空ss
      ss.str("");
      ss << node[i];
      vec.push_back(LexicalCast<std::string, T>()(ss.str()));
    }
    return vec;
  }
};

int main() {
  // // 示例：将字符串转换为整数
  // LexicalCast<std::string, int> lexicalCast;
  // std::string str = "123";
  // int num = lexicalCast(str);
  // std::cout << num << std::endl;  // 输出: 123

  // 示例：将字符串转换为整数， 还可以这样使用
  std::string str = "123";
  auto num = LexicalCast<std::string, int>()(str);
  std::cout << num << std::endl;  // 输出: 123

  // 示例：将字符串转换为整数向量
  LexicalCast<std::string, std::vector<int>> lexicalCast2;
  std::string str2 = "[1, 2, 3, 4, 5]";
  std::vector<int> vec = lexicalCast2(str);
  for (const auto &num : vec) {
    std::cout << num << " ";
  }
  // 输出: 1 2 3 4 5

  // 示例：将字符串转换为整数向量 还可以这样使用
  std::string str3 = "[1, 2, 3, 4, 5]";
  std::vector<int> vec2 = LexicalCast<std::string, std::vector<int>>()(str3);
  for (const auto &num : vec2) {
    std::cout << num << " ";
  }
  // 输出: 1 2 3 4 5

  std::vector<int> v =
      LexicalCast<std::string, std::vector<int>>()("[1, 2, 3]");
  // ok, v等于[1, 2, 3]
  return 0;
}

// int main() {
//   std::string str1 =
//       sylar::LexicalCast<int, std::string>()(123);  // ok, str1等于"123"
//   int int1 = sylar::LexicalCast<std::string, int>()("123");  // ok,
//   int1等于123 std::string str2 =
//       sylar::LexicalCast<float, std::string>()(3.14);  // ok，str2等于"3.14"
//   float float2 =
//       sylar::LexicalCast<std::string, float>()("3.14");  //
//       ok，float2等于3.14

//   std::cout << "int1:" << int1 << std::endl;
//   std::cout << "float2:" << float2 << std::endl;

//   std::vector<int> v = sylar::LexicalCast<std::string, std::vector<int>>()(
//       "[1, 2, 3]");  // ok, v等于[1, 2, 3]
//   std::string s = sylar::LexicalCast<std::vector<int>, std::string>()(
//       std::vector<int>{1, 2, 3});

//   std::cout << "vector:";
//   for (auto it : v) {
//     std::cout << it;
//   }
//   std::cout << "string:" << s << std::endl;

//   std::vector<std::vector<int>> vv =
//       sylar::LexicalCast<std::string, std::vector<std::vector<int>>>()(
//           "[[1,2,3],[4,5,6]]");
//   std::string ss =
//       sylar::LexicalCast<std::vector<std::vector<int>>, std::string>()(vv);

//   std::cout << "vector<vector<int>>:" << std::endl;
//   for (auto it : vv) {
//     for (auto i : it) {
//       std::cout << i;
//     }
//     std::cout << "\n";
//   }
//   std::cout << "string:" << ss << std::endl;
//   return 0;
// }