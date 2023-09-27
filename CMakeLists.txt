cmake_minimum_required(VERSION 3.22.1)
project(sylar)

set(CMAKE_VERBOSE_MAKEFILE ON)

##设置 生成compile_commands.json 提供跳转功能 
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic  -O3  -g -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function")
set(THIRD_PARTY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party)

# boost
list(APPEND CMAKE_PREFIX_PATH ${THIRD_PARTY_DIR}/boost)
find_package(Boost REQUIRED) # COMPONENTS ALL
message(STATUS "Find Boost in ${Boost_DIR}")
message(STATUS "Boost version: ${Boost_VERSION}")

# yaml-cpp
list(APPEND CMAKE_PREFIX_PATH ${THIRD_PARTY_DIR}/yaml-cpp)
find_package(yaml-cpp REQUIRED) # COMPONENTS ALL
message(STATUS "Find yaml-cpp in ${yaml-cpp_DIR}")
message(STATUS "yaml-cpp version: ${yaml-cpp_VERSION}")
set(yaml-cpp_LIBRARIES ${THIRD_PARTY_DIR}/yaml-cpp/lib/libyaml-cpp.a)
set(yaml-cpp_INCLUDE_DIRS ${THIRD_PARTY_DIR}/yaml-cpp/include)

if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif()

include_directories(${yaml-cpp_INCLUDE_DIRS})

set(LIB_SRC
    sylar/log.cc
    sylar/util.cc
    sylar/config.cc
    )

add_library(sylar SHARED ${LIB_SRC})
target_link_libraries(sylar PUBLIC ${Boost_LIBRARIES} ${yaml-cpp_LIBRARIES})
target_include_directories(sylar PUBLIC ${yaml-cpp_INCLUDE_DIRS})

# add_library(sylar_static STATIC ${LIB_SRC})
# SET_TARGET_PROPERTIES(sylar_static PROPERTIES OUTPUT_NAME "sylar")
#
add_executable(test tests/test.cc)
# add_dependencies(test sylar)
target_link_libraries(test sylar)

add_executable(test_config tests/test_config.cc )
target_include_directories(test_config PRIVATE  ${yaml-cpp_INCLUDE_DIRS})
target_link_libraries(test_config  PRIVATE sylar ${yaml-cpp_LIBRARIES})


# 可执行文件输出位置
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
# lib文件输出位置
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)