add_executable(StatusServer
  StatusServer.cpp
  StatusServiceImpl.cpp)
target_link_libraries(
  StatusServer PRIVATE backend_core spdlog::spdlog ${Boost_LIBRARIES} ${JSONCPP_LIBRARIES}
                     ${_GRPC_GRPCPP}  ${HIREDIS_LIBRARIES})
