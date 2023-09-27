#include <yaml-cpp/yaml.h>
#include "../sylar/config.h"
#include "../sylar/log.h"
#include "yaml-cpp/node/node.h"

sylar::ConfigVar<int>::ptr g_int_value_config =
    sylar::Config::Lookup("system.port", (int)8080, "system port");

void print_yaml() { YAML::Node root; }
void test_yaml() {
  YAML::Node root = YAML::LoadFile("../template/bin/conf/log.yaml");
  SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << root;
}
int main(int argc, char **argv) {
  SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << g_int_value_config->getValue();
  SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << g_int_value_config->toString();
  test_yaml();
}