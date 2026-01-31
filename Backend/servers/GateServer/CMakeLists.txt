add_executable(GateServer main.cpp)
target_link_libraries(
  GateServer PRIVATE backend_core spdlog::spdlog ${Boost_LIBRARIES} ${JSONCPP_LIBRARIES}
                     ${_GRPC_GRPCPP}  ${HIREDIS_LIBRARIES})
# add_custom_target(run COMMAND ${CMAKE_BINARY_DIR}/servers/GateServer/G  #
# 替换成你的可执行文件名 DEPENDS GateServer WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}        #
# 确保 logs/ 在根目录生成 USES_TERMINAL                                 #
# 允许程序在控制台实时打印彩色日志 )
