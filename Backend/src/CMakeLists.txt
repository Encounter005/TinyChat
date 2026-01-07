file(GLOB_RECURSE srcs CONFIGURE_DEPENDS *.cpp)

add_library(backend_core STATIC ${srcs})

target_include_directories(
  backend_core
  PUBLIC ${CMAKE_SOURCE_DIR}/include # 让别人能 #include "CServer.h"
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} # 只有内部能看到 .cpp 同级的 .h
          ${HIREDIS_INCLUDE_DIRS} ${SPDLOG_INCLUDE_DIRS})

# 链接依赖：让 backend_core 自动获得 proto 的头文件
target_link_libraries(
  backend_core PUBLIC proto_gen ${Boost_LIBRARIES} spdlog::spdlog
                      ${HIREDIS_LIBRARIES} Threads::Threads)
