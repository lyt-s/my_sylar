# 可执行文件输出位置
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin/tests)
# # 可执行文件输出位置 build/bin/test
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/tests)
#----test-----------------------------------------------------------------------
add_executable(test test.cc)
add_dependencies(test sylar)
force_redefine_file_macro_for_sources(test) #__FILE__
target_link_libraries(test sylar)
target_include_directories(test PRIVATE ${CMAKE_SOURCE_DIR})
#----test_config----------------------------------------------------------------
add_executable(test_config test_config.cc )
add_dependencies(test_config sylar)
force_redefine_file_macro_for_sources(test_config) #__FILE__
target_include_directories(test_config  PRIVATE  
                                        ${CMAKE_SOURCE_DIR}
                                        ${yaml-cpp_INCLUDE_DIRS} 
                                        ${Boost_INCLUDE_DIRS})
target_link_libraries(test_config   PRIVATE sylar)
#----test_thread----------------------------------------------------------------
add_executable(test_thread test_pthread.cc)
force_redefine_file_macro_for_sources(test_thread) #__FILE__
target_include_directories(test_thread PRIVATE  ${CMAKE_SOURCE_DIR}
                                                ${yaml-cpp_INCLUDE_DIRS} 
                                                ${Boost_INCLUDE_DIRS})


target_link_libraries(test_thread   PRIVATE sylar)
#----test_lexical_cast----------------------------------------------------------
add_executable(test_lexical_cast test_lexical_cast.cc)
force_redefine_file_macro_for_sources(test_lexical_cast) #__FILE__
target_include_directories(test_lexical_cast PRIVATE  ${CMAKE_SOURCE_DIR}
                                                ${yaml-cpp_INCLUDE_DIRS} 
                                                ${Boost_INCLUDE_DIRS})


target_link_libraries(test_lexical_cast PRIVATE sylar)

#----test_fiber-----------------------------------------------------------------
add_executable(test_fiber test_fiber.cc)
force_redefine_file_macro_for_sources(test_fiber) #__FILE__
target_include_directories(test_fiber PRIVATE  ${CMAKE_SOURCE_DIR}
                                                ${yaml-cpp_INCLUDE_DIRS} 
                                                ${Boost_INCLUDE_DIRS})


target_link_libraries(test_fiber PRIVATE sylar)
#----test-----------------------------------------------------------------------
add_executable(test_schedule test_schedule.cc)
force_redefine_file_macro_for_sources(test_schedule) #__FILE__
target_include_directories(test_schedule PRIVATE  ${CMAKE_SOURCE_DIR}
                                                ${yaml-cpp_INCLUDE_DIRS} 
                                                ${Boost_INCLUDE_DIRS})


target_link_libraries(test_schedule PRIVATE  sylar)
#----test-----------------------------------------------------------------------
add_executable(test_iomanager test_iomanager.cc)
force_redefine_file_macro_for_sources(test_iomanager) #__FILE__
target_include_directories(test_iomanager PRIVATE  ${CMAKE_SOURCE_DIR}
                                                ${yaml-cpp_INCLUDE_DIRS} 
                                                ${Boost_INCLUDE_DIRS})


target_link_libraries(test_iomanager PRIVATE sylar)
#----test-----------------------------------------------------------------------
add_executable(test_hook test_hook.cc)
force_redefine_file_macro_for_sources(test_hook) #__FILE__
target_include_directories(test_hook PRIVATE  ${CMAKE_SOURCE_DIR}
                                                ${yaml-cpp_INCLUDE_DIRS} 
                                                ${Boost_INCLUDE_DIRS})


target_link_libraries(test_hook PRIVATE  sylar)
#----test-----------------------------------------------------------------------
add_executable(test_address test_address.cc)
force_redefine_file_macro_for_sources(test_address) #__FILE__
target_include_directories(test_address PRIVATE  ${CMAKE_SOURCE_DIR}
                                                ${yaml-cpp_INCLUDE_DIRS} 
                                                ${Boost_INCLUDE_DIRS})


target_link_libraries(test_address PRIVATE   sylar)
#----test-----------------------------------------------------------------------
add_executable(test_socket test_socket.cc)
force_redefine_file_macro_for_sources(test_socket) #__FILE__
target_include_directories(test_socket PRIVATE  ${CMAKE_SOURCE_DIR}
                                                ${yaml-cpp_INCLUDE_DIRS} 
                                                ${Boost_INCLUDE_DIRS})

            

target_link_libraries(test_socket PRIVATE sylar)
#----test-----------------------------------------------------------------------
add_executable(test_byte_array test_byte_array.cc)
force_redefine_file_macro_for_sources(test_byte_array) #__FILE__
target_include_directories(test_byte_array PRIVATE  ${CMAKE_SOURCE_DIR}
                                                ${yaml-cpp_INCLUDE_DIRS} 
                                                ${Boost_INCLUDE_DIRS})

            

target_link_libraries(test_byte_array PRIVATE  sylar)
#----test-----------------------------------------------------------------------
add_executable(test_http test_http.cc)
force_redefine_file_macro_for_sources(test_http) #__FILE__
target_include_directories(test_http PRIVATE  ${CMAKE_SOURCE_DIR}
                                                ${yaml-cpp_INCLUDE_DIRS} 
                                                ${Boost_INCLUDE_DIRS})

            

target_link_libraries(test_http PRIVATE sylar )
#----test-----------------------------------------------------------------------
add_executable(test_http_parser test_http_parser.cc)
force_redefine_file_macro_for_sources(test_http_parser) #__FILE__
target_include_directories(test_http_parser PRIVATE  ${CMAKE_SOURCE_DIR}
                                                ${yaml-cpp_INCLUDE_DIRS} 
                                                ${Boost_INCLUDE_DIRS})

            

target_link_libraries(test_http_parser PRIVATE  sylar)
#----test-----------------------------------------------------------------------
add_executable(test_tcp_server test_tcp_server.cc)
force_redefine_file_macro_for_sources(test_tcp_server) #__FILE__
target_include_directories(test_tcp_server PRIVATE  ${CMAKE_SOURCE_DIR}
                                                ${yaml-cpp_INCLUDE_DIRS} 
                                                ${Boost_INCLUDE_DIRS})

            

target_link_libraries(test_tcp_server PRIVATE  sylar)
#----test-----------------------------------------------------------------------
add_executable(test_http_server test_http_server.cc)

target_include_directories(test_http_server PRIVATE  ${CMAKE_SOURCE_DIR}
                                                ${yaml-cpp_INCLUDE_DIRS} 
                                                ${Boost_INCLUDE_DIRS})

            

target_link_libraries(test_http_server PRIVATE sylar)
#----test-----------------------------------------------------------------------
add_executable(test_http_connection test_http_connection.cc)

target_include_directories(test_http_connection PRIVATE  ${CMAKE_SOURCE_DIR}
                                                ${yaml-cpp_INCLUDE_DIRS} 
                                                ${Boost_INCLUDE_DIRS})

            

target_link_libraries(test_http_connection PRIVATE sylar)
#----test-----------------------------------------------------------------------
add_executable(test_uri test_uri.cc)

target_include_directories(test_uri PRIVATE  ${CMAKE_SOURCE_DIR}
                                                ${yaml-cpp_INCLUDE_DIRS} 
                                                ${Boost_INCLUDE_DIRS})

            

target_link_libraries(test_uri PRIVATE sylar)

#----test-----------------------------------------------------------------------
add_executable(test_daemon test_daemon.cc)

target_include_directories(test_daemon PRIVATE  ${CMAKE_SOURCE_DIR}
                                                ${yaml-cpp_INCLUDE_DIRS} 
                                                ${Boost_INCLUDE_DIRS})

            

target_link_libraries(test_daemon PRIVATE  sylar)
#--------------------------------------------------------------------------------
add_executable(test_env test_env.cc)

target_include_directories(test_env PRIVATE  ${CMAKE_SOURCE_DIR}
                                                ${yaml-cpp_INCLUDE_DIRS} 
                                                ${Boost_INCLUDE_DIRS})

            

target_link_libraries(test_env PRIVATE  sylar)
#--------------------------------------------------------------------------------
add_executable(test_application test_application.cc)

target_include_directories(test_application PRIVATE  ${CMAKE_SOURCE_DIR}
                                                ${yaml-cpp_INCLUDE_DIRS} 
                                                ${Boost_INCLUDE_DIRS})

            

target_link_libraries(test_application PRIVATE  sylar)
#--------------------------------------------------------------------------------
