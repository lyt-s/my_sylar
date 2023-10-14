
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
target_link_libraries(test_config   PRIVATE 
                                    sylar 
                                    ${Boost_LIBRARIES}
                                    ${yaml-cpp_LIBRARIES})
#----test_thread----------------------------------------------------------------
add_executable(test_thread test_pthread.cc)
force_redefine_file_macro_for_sources(test_thread) #__FILE__
target_include_directories(test_thread PRIVATE  ${CMAKE_SOURCE_DIR}
                                                ${yaml-cpp_INCLUDE_DIRS} 
                                                ${Boost_INCLUDE_DIRS})


target_link_libraries(test_thread PRIVATE pthread sylar ${Boost_LIBRARIES}
${yaml-cpp_LIBRARIES})
#----test_lexical_cast---------------------------------------------------------------------------
add_executable(test_lexical_cast test_lexical_cast.cc)

target_include_directories(test_lexical_cast PRIVATE  ${CMAKE_SOURCE_DIR}
                                                ${yaml-cpp_INCLUDE_DIRS} 
                                                ${Boost_INCLUDE_DIRS})


target_link_libraries(test_lexical_cast PRIVATE pthread sylar ${Boost_LIBRARIES})

#----test_thread----------------------------------------------------------------
add_executable(test_fiber test_fiber.cc)

target_include_directories(test_fiber PRIVATE  ${CMAKE_SOURCE_DIR}
                                                ${yaml-cpp_INCLUDE_DIRS} 
                                                ${Boost_INCLUDE_DIRS})


target_link_libraries(test_fiber PRIVATE pthread sylar ${Boost_LIBRARIES})
#----test-----------------------------------------------------------------------
add_executable(test_schedule test_schedule.cc)

target_include_directories(test_schedule PRIVATE  ${CMAKE_SOURCE_DIR}
                                                ${yaml-cpp_INCLUDE_DIRS} 
                                                ${Boost_INCLUDE_DIRS})


target_link_libraries(test_schedule PRIVATE pthread sylar ${Boost_LIBRARIES})