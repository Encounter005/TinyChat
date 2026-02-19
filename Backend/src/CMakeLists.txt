message(STATUS "[Configuring] Core Library (backend_core)")

file(GLOB_RECURSE srcs CONFIGURE_DEPENDS
    core/*.cpp
    controller/*.cpp
    service/*.cpp
    dao/*.cpp
    infra/*.cpp
    common/*.cpp
    grpcClient/*.cpp
    repository/*.cpp
)

add_library(backend_core STATIC ${srcs})

message(STATUS "[Target]      backend_core (STATIC LIBRARY)")

target_include_directories(backend_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE
        ${HIREDIS_INCLUDE_DIRS}
        ${SPDLOG_INCLUDE_DIRS}
)

target_link_libraries(backend_core
    PUBLIC
        proto_gen
        ${Boost_LIBRARIES}
        spdlog::spdlog
        ${MYSQL_LIBRARY}
        ${HIREDIS_LIBRARIES}
        Threads::Threads
)
