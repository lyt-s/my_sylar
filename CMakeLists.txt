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
set(LIB_SRC
    sylar/config.cc
    sylar/fiber.cc
    sylar/log.cc
    sylar/util.cc
    sylar/thread.cc
    )

add_library(sylar SHARED ${LIB_SRC})
force_redefine_file_macro_for_sources(sylar) #__FILE__
target_link_libraries(sylar PUBLIC 
                            pthread
                            ${Boost_LIBRARIES}
                            ${yaml-cpp_LIBRARIES})

target_include_directories(sylar    PUBLIC   
                                    ${CMAKE_CURRENT_SOURCE_DIR}/sylar
                                    ${yaml-cpp_INCLUDE_DIRS} 
                                    ${Boost_INCLUDE_DIRS})


# add_library(sylar_static STATIC ${LIB_SRC})
# SET_TARGET_PROPERTIES(sylar_static PROPERTIES OUTPUT_NAME "sylar")
#

#----test-----------------------------------------------------------------------
add_executable(test tests/test.cc)
add_dependencies(test sylar)
force_redefine_file_macro_for_sources(test) #__FILE__
target_link_libraries(test PRIVATE sylar)
target_include_directories(test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

#----test_config----------------------------------------------------------------
add_executable(test_config tests/test_config.cc )
add_dependencies(test_config sylar)
force_redefine_file_macro_for_sources(test_config) #__FILE__
target_include_directories(test_config  PRIVATE  
                                        ${CMAKE_CURRENT_SOURCE_DIR}
                                        ${yaml-cpp_INCLUDE_DIRS} 
                                        ${Boost_INCLUDE_DIRS})
target_link_libraries(test_config   PRIVATE 
                                    sylar 
                                    ${Boost_LIBRARIES}
                                    ${yaml-cpp_LIBRARIES})
#----test_thread----------------------------------------------------------------
add_executable(test_thread tests/test_pthread.cc)
force_redefine_file_macro_for_sources(test_thread) #__FILE__
target_include_directories(test_thread PRIVATE  ${CMAKE_CURRENT_SOURCE_DIR}
                                                ${yaml-cpp_INCLUDE_DIRS} 
                                                ${Boost_INCLUDE_DIRS})


target_link_libraries(test_thread PRIVATE pthread sylar ${Boost_LIBRARIES}
${yaml-cpp_LIBRARIES})
#----test_lexical_cast----------------------------------------------------------
add_executable(test_lexical_cast tests/test_lexical_cast.cc)

target_include_directories(test_lexical_cast PRIVATE  ${CMAKE_CURRENT_SOURCE_DIR}
                                                ${yaml-cpp_INCLUDE_DIRS} 
                                                ${Boost_INCLUDE_DIRS})


target_link_libraries(test_lexical_cast PRIVATE pthread sylar ${Boost_LIBRARIES})

#----test_util------------------------------------------------------------------
add_executable(test_util tests/test_util.cc)

target_include_directories(test_util PRIVATE  ${CMAKE_CURRENT_SOURCE_DIR}
                                                ${yaml-cpp_INCLUDE_DIRS} 
                                                ${Boost_INCLUDE_DIRS})


target_link_libraries(test_util PRIVATE pthread sylar ${Boost_LIBRARIES})

#----test_fiber-----------------------------------------------------------------------
add_executable(test_fiber tests/test_fiber.cc)

target_include_directories(test_fiber PRIVATE  ${CMAKE_CURRENT_SOURCE_DIR}
                                                ${yaml-cpp_INCLUDE_DIRS} 
                                                ${Boost_INCLUDE_DIRS})


target_link_libraries(test_fiber PRIVATE pthread sylar ${Boost_LIBRARIES})
#----test-----------------------------------------------------------------------
# 可执行文件输出位置
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
# lib文件输出位置
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

# Set output folders for targets.
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
# set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
# set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)