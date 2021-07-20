cmake_minimum_required(VERSION 3.11 FATAL_ERROR)

project(cloudcompute)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(gRPC REQUIRED)
find_package(Boost REQUIRED COMPONENTS random serialization thread)
find_package(OpenCL REQUIRED)


include(FetchContent)

FetchContent_Declare(drogon
  GIT_REPOSITORY https://github.com/an-tao/drogon.git
  GIT_TAG        v1.7.1
)
set(BUILD_PROGRAMS OFF CACHE BOOL "Disable programs" FORCE)
set(BUILD_ORM OFF CACHE BOOL "Disable orm" FORCE)
set(BUILD_CTL OFF CACHE BOOL "Disable ctl" FORCE)
FetchContent_GetProperties(drogon)
if(NOT drogon_POPULATED)
  FetchContent_Populate(drogon)
  add_subdirectory(${drogon_SOURCE_DIR} ${drogon_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

#set(BUILD_SHARED_LIBS OFF)
#FetchContent_Declare(restbed
#  #GIT_REPOSITORY https://github.com/Corvusoft/restbed.git
#  #GIT_TAG        5817f080886ce72b91afe390aa9c274b5a8b0da1
#  URL            https://github.com/Corvusoft/restbed/archive/refs/tags/4.7.tar.gz
#)
#FetchContent_GetProperties(restbed)
#if(NOT restbed_POPULATED)
#  FetchContent_Populate(restbed)
#
#  set(BUILD_TESTS FALSE CACHE BOOL "Build unit tests")
#  set(BUILD_SSL OFF CACHE BOOL "Disable building SSL libraries")
#  add_subdirectory(${restbed_SOURCE_DIR} ${restbed_BINARY_DIR} EXCLUDE_FROM_ALL)
#endif()

add_library(compute
    src/kernel.cpp
    src/kernel.h
)

grpc_add_protocol(compute src/compute_kernel.proto)

target_compile_definitions(compute PUBLIC
    CL_TARGET_OPENCL_VERSION=220
)

target_link_libraries(compute PUBLIC
    gRPC::gRPC
    Boost::random
    Boost::serialization
    Boost::thread
    OpenCL::OpenCL
    drogon
)

add_executable(compute_client
    src/rpc_client.cpp
)

add_executable(compute_server
    src/rpc_server.cpp
)

target_link_libraries(compute_client PRIVATE
    compute
)

target_link_libraries(compute_server PRIVATE
    compute
)

add_executable(http_server
    src/http_server.cpp
    src/server.h
    src/server.cpp
)
target_link_libraries(http_server PRIVATE
    drogon
    compute
)

#add_executable(http_server_2 src/http_server_2.cpp)
#target_link_libraries(http_server_2 PRIVATE restbed-static)
