cmake_minimum_required (VERSION 2.4)

project (fbench)

# Specialised compiler flags such as WORKERS, FEATURES or INTERACTIONS, TABLES or ATTRIBUTES can be set. For instance "-DWORKERS=2". These must be consistent with the values in the configuration files.
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -std=c++11 -pthread -g -Wall -Wextra -Og")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -std=c++11 -pthread -Ofast -mtune=native -fassociative-math -freciprocal-math -fno-signed-zeros -frename-registers -fopenmp")
else()
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -std=c++11 -pthread -g -Wall -Wextra -O0")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -std=c++11")
endif()
set(CMAKE_CXX_FLAGS_BENCHMARK "${CMAKE_CXX_FLAGS_RELEASE} -DBENCH")

# Set to Debug, Benchmark or Release.
set(CMAKE_BUILD_TYPE Release)

#include_directories(include/database include/dTree include/engine include/model include/result include/launcher include/utils libs/murmurhash3 /usr/local/include)
include_directories(include/database include/dTree include/engine include/model include/launcher include/utils /usr/local/include)

add_subdirectory(src/database)
add_subdirectory(src/engine)
add_subdirectory(src/model)
#add_subdirectory(src/network)
#add_subdirectory(src/result)
add_subdirectory(src/launcher)
add_subdirectory(src/dTree)
#add_subdirectory(libs/murmurhash3)

add_executable (fbench fbench.cpp)
#target_link_libraries (fbench launcher database model engine result murmurhash3 dTree)
target_link_libraries (fbench launcher database model engine dTree)

# Take care of Boost libraries.
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost COMPONENTS system iostreams program_options)

if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    target_link_libraries(fbench ${Boost_LIBRARIES})
endif()
