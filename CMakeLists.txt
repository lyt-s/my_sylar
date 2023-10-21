cmake_minimum_required(VERSION 3.22.1)
project(sylar)

set(CMAKE_VERBOSE_MAKEFILE ON)

# 修改log生成绝对路径改为相对路径  重定义 __FILE__
include(cmake/utils.cmake)

set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic  -O3  -g -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined")
#-------------------------------------------------------------------------------
# Find packages in the './third_party' directory
include(cmake/FindPackages.cmake)

# set(LIB_LIB "${PTHREAD} ")
# message("******", ${LIB_LIB})
# Output compile_commands.json for clangd.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set C++ standard.
# set(CMAKE_CXX_STANDARD 11)
#-------------------------------------------------------------------------------
file(GLOB_RECURSE LIB_SRC sylar/*.cc)

add_library(sylar SHARED ${LIB_SRC})
force_redefine_file_macro_for_sources(sylar) #__FILE__
target_link_libraries(sylar PUBLIC 
                            dl # todo
                            pthread
                            ${Boost_LIBRARIES}
                            ${yaml-cpp_LIBRARIES})

target_include_directories(sylar    PUBLIC   
                                    ${CMAKE_CURRENT_SOURCE_DIR}/sylar
                                    ${yaml-cpp_INCLUDE_DIRS} 
                                    ${Boost_INCLUDE_DIRS})


# add_library(sylar_static STATIC ${LIB_SRC})
# SET_TARGET_PROPERTIES(sylar_static PROPERTIES OUTPUT_NAME "sylar"
# 可执行文件输出位置
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
# lib文件输出位置
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

# Set output folders for targets.
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
# set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
# set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# test
add_subdirectory(tests)