cmake_minimum_required(VERSION 3.5)
project(Backend)

# =============================================================================
# Build Configuration Summary
# =============================================================================
message(STATUS "")
message(STATUS "============================================================================")
message(STATUS "TinyChat Backend - Build Configuration")
message(STATUS "============================================================================")
message(STATUS "Build Directory:       ${CMAKE_BINARY_DIR}")
message(STATUS "Source Directory:      ${CMAKE_SOURCE_DIR}")
message(STATUS "C++ Standard:          C++${CMAKE_CXX_STANDARD}")
message(STATUS "Compiler:              ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Build Type:            ${CMAKE_BUILD_TYPE}")

#compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# setting compiler
set(CMAKE_CXX_COMPILER "g++")
# setting C++ Standard
set(CMAKE_CXX_STANDARD 17)

message(STATUS "Compile Flags:         -g -O1 -fcoroutines -Wall -Wextra -fsanitize=address")
message(STATUS "")

add_compile_options(
    -g
    -O1
    -fcoroutines
    -Wall
    -Wextra
    -Weffc++
    -Werror=return-type
    -Wconversion
    -Wsign-compare
    -Werror=unused-result
    -Werror=suggest-override
    -Wzero-as-null-pointer-constant
    -Wmissing-declarations
    -Wold-style-cast
    -Wnon-virtual-dtor
    -fsanitize=address
    -fno-omit-frame-pointer
)

add_link_options(
    -fsanitize=address)


find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
# jsoncpp library
pkg_check_modules(JSONCPP jsoncpp)
# Protobuf library
find_package(PROTOBUF REQUIRED protobuf)
message("Using protobuf: ${PROTOBUF_VERSION}")

# Grpc library
find_package(gRPC REQUIRED)
message(STATUS "gRPC Version:          ${gRPC_VERSION}")
set(_GRPC_GRPCPP gRPC::grpc++)

# boost library
set(Boost_USE_STATIC_LIBS OFF) # Linux上建议先用OFF，除非你确定有.a文件
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

find_package(Boost 1.89.0 COMPONENTS filesystem REQUIRED)

if(Boost_FOUND)
    message(STATUS "Boost Version:         ${Boost_VERSION}")
    message(STATUS "Boost Components:      filesystem")
else()
    message(FATAL_ERROR "Could not find Boost!")
endif()

#spdlog
# pkg_check_modules(SPDLOG REQUIRED spdlog)
find_package(spdlog CONFIG REQUIRED)
message(STATUS "spdlog:                FOUND")

# Redis library
pkg_check_modules(HIREDIS REQUIRED hiredis)
message(STATUS "hiredis:               FOUND")

#openssl
find_package(OpenSSL REQUIRED)
message(STATUS "OpenSSL:               FOUND")

#mysql
# 1. 寻找头文件路径
find_path(MYSQL_INCLUDE_DIR mysql_connection.h
    PATH_SUFFIXES cppconn
)
# 2. 寻找库文件
# 注意：mysqlcppconn 是传统的 JDBC 接口库
find_library(MYSQL_LIBRARY NAMES mysqlcppconn)

# 3. 打印调试信息（可选，方便你排查 Ninja 报错）
message(STATUS "MySQL Connector/C++:   FOUND")

# 4. 检查是否找到
if(NOT MYSQL_INCLUDE_DIR OR NOT MYSQL_LIBRARY)
    message(FATAL_ERROR "MySQL Connector/C++ not found!")
endif()

message(STATUS "")
message(STATUS "============================================================================")
message(STATUS "Dependencies Configuration Complete")
message(STATUS "============================================================================")
message(STATUS "")

# setting source code directory
message(STATUS "Configuring project subdirectories...")
message(STATUS "")
add_subdirectory(proto)
add_subdirectory(src)
add_subdirectory(servers)
add_subdirectory(TestCases)

message(STATUS "")
message(STATUS "============================================================================")
message(STATUS "Configuration Summary Complete")
message(STATUS "============================================================================")
message(STATUS "To build, run: make -j$(nproc)")
message(STATUS "============================================================================")
message(STATUS "")
