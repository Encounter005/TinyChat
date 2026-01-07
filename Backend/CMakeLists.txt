cmake_minimum_required(VERSION 3.5)
project(Backend)

#compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# setting compiler
set(CMAKE_CXX_COMPILER "g++")
# setting C++ Standard
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -g -fcoroutines -O3 -Wall -Wextra -Weffc++ -Werror=uninitialized  -Werror=return-type -Wconversion -Wsign-compare -Werror=unused-result -Werror=suggest-override -Wzero-as-null-pointer-constant -Wmissing-declarations -Wold-style-cast -Wnon-virtual-dtor -Wl,--copy-dt-needed-entries"
)

find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
# jsoncpp library
pkg_check_modules(JSONCPP jsoncpp)
# Protobuf library
find_package(PROTOBUF REQUIRED protobuf)
message("Using protobuf: ${PROTOBUF_VERSION}")

# Grpc library
find_package(gRPC REQUIRED)
message("Using gRPC: ${gRPC_VERSION}")
set(_GRPC_GRPCPP gRPC::grpc++)

# boost library
set(Boost_USE_STATIC_LIBS OFF) # Linux上建议先用OFF，除非你确定有.a文件
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

find_package(Boost 1.89.0 COMPONENTS filesystem REQUIRED)

if(Boost_FOUND)
    message(STATUS "Boost found!")
    message(STATUS "Boost include dirs: ${Boost_INCLUDE_DIRS}")
    message(STATUS "Boost library dirs: ${Boost_LIBRARY_DIRS}")
else()
    message(FATAL_ERROR "Could not find Boost!")
endif()

#spdlog
# pkg_check_modules(SPDLOG REQUIRED spdlog)
find_package(spdlog CONFIG REQUIRED)
# Redis library
pkg_check_modules(HIREDIS REQUIRED hiredis)

# find_path(HIREDIS_INCLUDE_DIR hiredis/hiredis.h)
# find_library(HIREDIS_LIBRARY hiredis)
# add_library(hiredis_lib INTERFACE)
# target_include_directories(hiredis_lib INTERFACE ${HIREDIS_INCLUDE_DIR})
# target_link_libraries(hiredis_lib INTERFACE ${HIREDIS_LIBRARY})

# setting source code directory
add_subdirectory(proto)
add_subdirectory(src)
add_subdirectory(servers)
add_subdirectory(TestCases)
