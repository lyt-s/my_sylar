set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

file(GLOB_RECURSE LIB_SRC ./*.cc)

add_library(sylar SHARED ${LIB_SRC})
force_redefine_file_macro_for_sources(sylar) #__FILE__

target_include_directories(sylar    PUBLIC   
                                    ${CMAKE_SOURCE_DIR}
                                    ${yaml_cpp_INCLUDE_DIRS} 
                                    ${Boost_INCLUDE_DIRS}
                                    ${json_cpp_INCLUDE_DIRS}
                                    ${zlib_INCLUDE_DIRS}
                                    ${protobuf_INCLUDE_DIRS}
                                    ${Redis_INCLUDE_DIRS}
                                    )

target_link_libraries(sylar PUBLIC 
                                    dl # todo  dslym
                                    pthread
                                    ${Boost_LIBRARIES}
                                    ${json_LIBRARIES}
                                    ${ZLIB_LIBRARIES}
                                    ${yaml_cpp_LIBRARIES}
                                    ${Protobuf_LIBRARIES}
                                    )

# add_library(sylar_static STATIC ${LIB_SRC})
# SET_TARGET_PROPERTIES(sylar_static PROPERTIES OUTPUT_NAME "sylar")