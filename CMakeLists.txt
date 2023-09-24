cmake_minimum_required(VERSION 3.22.1)
project(sylar)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic  -O3  -g -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function")

set(LIB_SRC
    sylar/log.cc)

add_library(sylar SHARED ${LIB_SRC})
# add_library(sylar_static STATIC ${LIB_SRC})
# SET_TARGET_PROPERTIES(sylar_static PROPERTIES OUTPUT_NAME "sylar")
#
add_executable(test tests/test.cc)
add_dependencies(test sylar)
target_link_libraries(test sylar)

# 可执行文件输出位置
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
# lib文件输出位置
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)