# 可执行文件输出位置
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin/examples)
# # 可执行文件输出位置 build/bin/examples
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/examples)
#----examples-----------------------------------------------------------------------
add_executable(echo_server echo_server.cc)
force_redefine_file_macro_for_sources(echo_server) #__FILE__
target_include_directories(echo_server PRIVATE  ${CMAKE_SOURCE_DIR}
                                                ${yaml-cpp_INCLUDE_DIRS} 
                                                ${Boost_INCLUDE_DIRS})

target_link_libraries(echo_server PRIVATE   sylar)