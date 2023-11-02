


file(GLOB_RECURSE LIB_SRC ./*.cc)
add_library(sylar SHARED ${LIB_SRC})
force_redefine_file_macro_for_sources(sylar) #__FILE__
target_link_libraries(sylar PUBLIC 
                            dl # todo  dslym
                            pthread
                            ${Boost_LIBRARIES}
                            ${yaml-cpp_LIBRARIES})

target_include_directories(sylar    PUBLIC   
                                    ${CMAKE_CURRENT_SOURCE_DIR}
                                    ${yaml-cpp_INCLUDE_DIRS} 
                                    ${Boost_INCLUDE_DIRS})


# add_library(sylar_static STATIC ${LIB_SRC})
# SET_TARGET_PROPERTIES(sylar_static PROPERTIES OUTPUT_NAME "sylar"