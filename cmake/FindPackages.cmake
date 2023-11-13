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

# zlib
list(APPEND CMAKE_PREFIX_PATH ${THIRD_PARTY_DIR}/zlib)
# find_package(zlib REQUIRED)  大小写问题？？
find_package(ZLIB REQUIRED) # COMPONENTS ALLs
message(STATUS "Find zlib in ${Zlib_DIR}")
message(STATUS "zlib version: ${Zlib_VERSION}")
