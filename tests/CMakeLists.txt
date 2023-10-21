
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
                                    dl
                                    ${Boost_LIBRARIES}
                                    ${yaml-cpp_LIBRARIES})
#----test_thread----------------------------------------------------------------
add_executable(test_thread test_pthread.cc)
force_redefine_file_macro_for_sources(test_thread) #__FILE__
target_include_directories(test_thread PRIVATE  ${CMAKE_SOURCE_DIR}
                                                ${yaml-cpp_INCLUDE_DIRS} 
                                                ${Boost_INCLUDE_DIRS})


target_link_libraries(test_thread   PRIVATE 
                                    sylar
                                    ${Boost_LIBRARIES}
                                    ${yaml-cpp_LIBRARIES})
#----test_lexical_cast----------------------------------------------------------
add_executable(test_lexical_cast test_lexical_cast.cc)
force_redefine_file_macro_for_sources(test_lexical_cast) #__FILE__
target_include_directories(test_lexical_cast PRIVATE  ${CMAKE_SOURCE_DIR}
                                                ${yaml-cpp_INCLUDE_DIRS} 
                                                ${Boost_INCLUDE_DIRS})


target_link_libraries(test_lexical_cast PRIVATE dl pthread sylar ${Boost_LIBRARIES})

#----test_fiber-----------------------------------------------------------------
add_executable(test_fiber test_fiber.cc)
force_redefine_file_macro_for_sources(test_fiber) #__FILE__
target_include_directories(test_fiber PRIVATE  ${CMAKE_SOURCE_DIR}
                                                ${yaml-cpp_INCLUDE_DIRS} 
                                                ${Boost_INCLUDE_DIRS})


target_link_libraries(test_fiber PRIVATE dl pthread sylar ${Boost_LIBRARIES})
#----test-----------------------------------------------------------------------
add_executable(test_schedule test_schedule.cc)
force_redefine_file_macro_for_sources(test_schedule) #__FILE__
target_include_directories(test_schedule PRIVATE  ${CMAKE_SOURCE_DIR}
                                                ${yaml-cpp_INCLUDE_DIRS} 
                                                ${Boost_INCLUDE_DIRS})


target_link_libraries(test_schedule PRIVATE dl pthread sylar ${Boost_LIBRARIES})
#----test-----------------------------------------------------------------------
add_executable(test_iomanager test_iomanager.cc)

target_include_directories(test_iomanager PRIVATE  ${CMAKE_SOURCE_DIR}
                                                ${yaml-cpp_INCLUDE_DIRS} 
                                                ${Boost_INCLUDE_DIRS})


target_link_libraries(test_iomanager PRIVATE dl pthread sylar ${Boost_LIBRARIES})
#----test-----------------------------------------------------------------------
add_executable(test_hook test_hook.cc)

target_include_directories(test_hook PRIVATE  ${CMAKE_SOURCE_DIR}
                                                ${yaml-cpp_INCLUDE_DIRS} 
                                                ${Boost_INCLUDE_DIRS})


target_link_libraries(test_hook PRIVATE  dl pthread sylar ${Boost_LIBRARIES})