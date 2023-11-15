cmake_minimum_required(VERSION 3.22.1)
project(sylar)

set(CMAKE_VERBOSE_MAKEFILE ON)

# // todo o0 O3
# set(CMAKE_C_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -fPIC -ggdb -std=c11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic  -O3  -fPIC -g -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")
#-------------------------------------------------------------------------------

# Find packages in the './third_party' directory
include(cmake/FindPackages.cmake)

# 修改log生成绝对路径改为相对路径  重定义 __FILE__
include(cmake/utils.cmake)

# message("******", ${LIB_LIB})

# Output compile_commands.json for clangd.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set C++ standard.
# set(CMAKE_CXX_STANDARD 11)
#-------------------------------------------------------------------------------


ragelmaker(sylar/uri.rl LIB_SRC ${CMAKE_SOURCE_DIR}/sylar)

ragelmaker(sylar/http/http11_parser.rl LIB_SRC ${CMAKE_SOURCE_DIR}/sylar/http)
ragelmaker(sylar/http/httpclient_parser.rl LIB_SRC ${CMAKE_SOURCE_DIR}/sylar/http)
# 可执行文件输出位置
# SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

# Set output folders for targets.
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
# set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
# set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

#lib
add_subdirectory(sylar)
# test
add_subdirectory(tests)
# add_subdirectory(examples)
# add_subdirectory(samples)
