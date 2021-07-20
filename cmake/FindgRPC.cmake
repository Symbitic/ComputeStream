#[=======================================================================[.rst:
FindgRPC
--------

gRPC libraries and helper functions.

IMPORTED Targets
^^^^^^^^^^^^^^^^

This module defines :prop_tgt:`IMPORTED` target ``gRPC::gRPC``, if
gRPC has been found.

Functions
^^^^^^^^^

.. command:: grpc_add_protocol

  .. code-block:: cmake

    grpc_add_protocol(<target> [<protocols>])

  For each protobuf file in ``[<protocols>]``, generate Protobuf and gRPC headers
  and sources, and add them to ``<target>``.

Example
^^^^^^^

.. code-block:: cmake

    cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

    project(helloworld)

    find_package(gRPC REQUIRED)

    add_executable(helloworld helloworld.cpp)

    grpc_add_protocol(helloworld helloworld.proto)

    target_link_libraries(helloworld PRIVATE gRPC::gRPC)


#]=======================================================================]

include(FindPackageHandleStandardArgs)
find_package(PkgConfig)

pkg_check_modules(_GRPC REQUIRED grpc++ grpc protobuf)
find_library(_GRPC_REFLECTION_LIBRARY grpc++_reflection)
find_program(_GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)
find_program(_PROTOBUF_PROTOC protoc)

find_package(Threads REQUIRED)
find_path(_GRPC_H_PATH grpc/grpc.h)
find_path(_GRPCPP_H_PATH grpcpp/grpcpp.h)
# grpc/grpc.h
# grpcpp/grpcpp.h
# GRPC: grpc++ grpc gpr z cares ssl crypto protobuf pthread
# Requires: gpr zlib libcares openssl
# _gRPC_FIND_ABSL
# _gRPC_FIND_RE2


if (NOT TARGET grpc)
    add_library(grpc INTERFACE)
    add_library(gRPC::gRPC ALIAS grpc)

    target_link_libraries(grpc INTERFACE ${_GRPC_LINK_LIBRARIES} ${_GRPC_REFLECTION_LIBRARY})
    target_include_directories(grpc INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    )
endif()

function(GRPC_ADD_PROTOCOL tgt)
    foreach(proto ${ARGN})
        get_filename_component(proto_file "${proto}" ABSOLUTE)
        get_filename_component(proto_path "${proto_file}" DIRECTORY)
        get_filename_component(proto_name "${proto_file}" NAME_WLE)

        # Generated sources
        set(proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/${proto_name}.pb.cc")
        set(proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/${proto_name}.pb.h")
        set(grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/${proto_name}.grpc.pb.cc")
        set(grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/${proto_name}.grpc.pb.h")

        # Generate sources from protocol file.
        add_custom_command(
            OUTPUT "${proto_srcs}" "${proto_hdrs}" "${grpc_srcs}" "${grpc_hdrs}"
            COMMAND ${_PROTOBUF_PROTOC}
            ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
                 --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
                 -I "${proto_path}"
                 --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
                 "${proto_file}"
            DEPENDS "${proto_file}"
        )

        target_sources(${tgt} PRIVATE
            ${proto_srcs}
            ${proto_hdrs}
            ${grpc_srcs}
            ${grpc_hdrs}
        )

        target_include_directories(${tgt} INTERFACE
            $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        )
    endforeach()
endfunction()

find_package_handle_standard_args(gRPC
    REQUIRED_VARS _GRPC_FOUND _GRPC_REFLECTION_LIBRARY _GRPC_CPP_PLUGIN_EXECUTABLE _PROTOBUF_PROTOC
    VERSION_VAR _GRPC_VERSION
)
