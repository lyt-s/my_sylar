# 可执行文件输出位置
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin/samples)
# # 可执行文件输出位置 build/bin/samples
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/samples)
#----examples-----------------------------------------------------------------------
add_executable(my_http_server my_http_server.cc)
force_redefine_file_macro_for_sources(my_http_server) #__FILE__

target_include_directories(my_http_server PRIVATE  ${CMAKE_SOURCE_DIR}
                                                ${yaml-cpp_INCLUDE_DIRS} 
                                                ${Boost_INCLUDE_DIRS})

target_link_libraries(my_http_server PRIVATE sylar)