cmake_minimum_required(VERSION 3.22.1)
project(sylar)

set(CMAKE_VERBOSE_MAKEFILE ON)


set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic  -O3  -g -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function")
#-------------------------------------------------------------------------------
# Find packages in the './third_party' directory
include(cmake/FindPackages.cmake)

# Output compile_commands.json for clangd.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set C++ standard.
# set(CMAKE_CXX_STANDARD 11)
#-------------------------------------------------------------------------------
set(LIB_SRC
    sylar/log.cc
    sylar/util.cc
    sylar/config.cc
    )

add_library(sylar SHARED ${LIB_SRC})
target_link_libraries(sylar PUBLIC 
                            ${Boost_LIBRARIES}
                            ${yaml-cpp_LIBRARIES})

target_include_directories(sylar    PUBLIC   
                                    ${CMAKE_CURRENT_SOURCE_DIR}/sylar
                                    ${yaml-cpp_INCLUDE_DIRS} 
                                    ${Boost_INCLUDE_DIRS})

# add_library(sylar_static STATIC ${LIB_SRC})
# SET_TARGET_PROPERTIES(sylar_static PROPERTIES OUTPUT_NAME "sylar")
#

#----test-----------------------------------------
add_executable(test tests/test.cc)
target_link_libraries(test sylar)

#----test_config------------------------------------------
add_executable(test_config tests/test_config.cc )
target_include_directories(test_config  PRIVATE  
                                        ${CMAKE_CURRENT_SOURCE_DIR}
                                        ${yaml-cpp_INCLUDE_DIRS} 
                                        ${Boost_INCLUDE_DIRS}${yaml-cpp_INCLUDE_DIRS})
target_link_libraries(test_config   PRIVATE 
                                    sylar 
                                    ${Boost_LIBRARIES}
                                    ${yaml-cpp_LIBRARIES})
#----------------------------------------------------------------------


#-------------------------------------------------------------------------------
# 可执行文件输出位置
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
# lib文件输出位置
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

# Set output folders for targets.
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
# set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
# set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)